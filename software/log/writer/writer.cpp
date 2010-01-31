#include "log/writer/writer.h"
#include "util/codec.h"
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <bzlib.h>



namespace {
	const std::size_t BLOCK_SIZE = 128 * 1024;

	std::string get_file_name(std::time_t stamp, const std::string &extension) {
		const std::string &base = Glib::get_user_data_dir();
		const std::string &sub = base + "/thunderbots";
		mkdir(sub.c_str(), 0777);
		std::tm *tim = std::localtime(&stamp);
		char timebuf[256];
		std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S %Z", tim);
		const std::string &file = sub + '/' + timebuf + '.' + extension;
		return file;
	}

	std::string get_log_file_name(std::time_t stamp) {
		return get_file_name(stamp, "log");
	}

	std::string get_index_file_name(std::time_t stamp) {
		return get_file_name(stamp, "idx");
	}

	bool is_ok_delta_score(int d) {
		return d == 0 || d == 1;
	}
}



log_writer::log_writer(clocksource &clksrc, field::ptr thefield, ball::ptr theball, team::ptr wteam, team::ptr eteam) : the_field(thefield), the_ball(theball), west_team(wteam), east_team(eteam), last_frame_time(std::time(0)), log_file(get_log_file_name(last_frame_time).c_str(), O_WRONLY | O_CREAT | O_EXCL), index_file(get_index_file_name(last_frame_time).c_str(), O_WRONLY | O_CREAT | O_EXCL), frame_count(0), byte_count(0), last_score_west(0), last_score_east(0) {
	clksrc.signal_tick().connect(sigc::mem_fun(*this, &log_writer::tick));
}



log_writer::~log_writer() {
	flush();
}



void log_writer::flush() {
	// If nothing is buffered up yet, just return.
	if (log_buffer.empty()) {
		return;
	}

	// This comes from the BZ2 documentation. To be guaranteed that
	// compressed data will fit in the buffer, allocate an output
	// buffer 1% larger than the input plus 600 extra bytes.
	uint8_t bzbuf[(log_buffer.size() * 101 + 99) / 100 + 600];
	unsigned int bzoutlen = sizeof(bzbuf);

	// Compress the current block.
	int rc = BZ2_bzBuffToBuffCompress(reinterpret_cast<char *>(bzbuf), &bzoutlen, reinterpret_cast<char *>(&log_buffer[0]), log_buffer.size(), 9, 0, 0);
	if (rc != BZ_OK) {
		throw std::runtime_error("Error compressing data!");
	}

	// Write the block to the log file.
	ssize_t ssz = write(log_file, bzbuf, bzoutlen);
	if (ssz != static_cast<ssize_t>(bzoutlen)) {
		throw std::runtime_error("Error writing to log file!");
	}

	// Update byte count to reflect newly written data.
	byte_count += bzoutlen;

	// Write an index record.
	std::vector<uint8_t> index_record;
	encode_u64(index_record, frame_count);
	encode_u64(index_record, byte_count);
	ssz = write(index_file, &index_record[0], index_record.size());
	if (ssz != static_cast<ssize_t>(index_record.size())) {
		throw std::runtime_error("Error writing to log index file!");
	}

	// Clear log buffer.
	log_buffer.clear();
}



void log_writer::tick() {
	// Take a timestamp.
	std::time_t now = std::time(0);
	std::time_t delta_time = now - last_frame_time;
	int delta_scores[2] = {west_team->score() - last_score_west, east_team->score() - last_score_east};
	last_frame_time = now;
	last_score_west = west_team->score();
	last_score_east = east_team->score();
	int16_t field_length = the_field->length() * 1000 + 0.49;
	int16_t field_total_length = the_field->total_length() * 1000 + 0.49;
	int16_t field_width = the_field->width() * 1000 + 0.49;
	int16_t field_total_width = the_field->total_width() * 1000 + 0.49;
	int16_t field_goal_width = the_field->goal_width() * 1000 + 0.49;
	int16_t field_centre_circle_radius = the_field->centre_circle_radius() * 1000 + 0.49;
	int16_t field_defense_area_radius = the_field->defense_area_radius() * 1000 + 0.49;
	int16_t field_defense_area_stretch = the_field->defense_area_stretch() * 1000 + 0.49;

	// Each frame record looks like this:
	//  1 byte:
	//   1 bit:  delta timestamp (0 or 1)
	//   7 bits: play type
	//  2 bytes: ball X position
	//  2 bytes: ball Y position
	//  For each team (west then east):
	//   1 byte:
	//    1 bit:  colour (1=yellow, 0=blue)
	//    1 bit:  delta score (1=increment, 0=no change)
	//    6 bits: # robots on team
	//   For each robot:
	//    2 bytes: X position
	//    2 bytes: Y position
	//    2 bytes: orientation
	//
	// Positions are measured in millimetres.
	// Orientations are measured in 10,000ths of radians.

	// We require, with no exceptions, that there be no more than 63 robots per
	// team.
	if (west_team->size() > 63 || east_team->size() > 63) {
		throw std::runtime_error("Team too big to log!");
	}

	// Check if any changes have happened that require starting a new block.
	bool new_header = false;
	new_header = new_header || (delta_time > 1);
	new_header = new_header || (!is_ok_delta_score(delta_scores[0]));
	new_header = new_header || (!is_ok_delta_score(delta_scores[1]));
	new_header = new_header || (field_length != last_field_length);
	new_header = new_header || (field_total_length != last_field_total_length);
	new_header = new_header || (field_width != last_field_width);
	new_header = new_header || (field_total_width != last_field_total_width);
	new_header = new_header || (field_goal_width != last_field_goal_width);
	new_header = new_header || (field_centre_circle_radius != last_field_centre_circle_radius);
	new_header = new_header || (field_defense_area_radius != last_field_defense_area_radius);
	new_header = new_header || (field_defense_area_stretch != last_field_defense_area_stretch);
	if (new_header) {
		flush();
	}

	// Write a header if needed.
	if (log_buffer.empty()) {
		last_frame_time = now;
		last_score_west = west_team->score();
		last_score_east = east_team->score();
		last_field_length = field_length;
		last_field_total_length = field_total_length;
		last_field_width = field_width;
		last_field_total_width = field_total_width;
		last_field_goal_width = field_goal_width;
		last_field_centre_circle_radius = field_centre_circle_radius;
		last_field_defense_area_radius = field_defense_area_radius;
		last_field_defense_area_stretch = field_defense_area_stretch;
		encode_u64(log_buffer, last_frame_time);
		encode_u64(log_buffer, frame_count);
		encode_u32(log_buffer, last_score_west);
		encode_u32(log_buffer, last_score_east);
		encode_u16(log_buffer, field_length);
		encode_u16(log_buffer, field_total_length);
		encode_u16(log_buffer, field_width);
		encode_u16(log_buffer, field_total_width);
		encode_u16(log_buffer, field_goal_width);
		encode_u16(log_buffer, field_centre_circle_radius);
		encode_u16(log_buffer, field_defense_area_radius);
		encode_u16(log_buffer, field_defense_area_stretch);
		delta_time = 0;
		delta_scores[0] = 0;
		delta_scores[1] = 0;
	}

	// Write the data for this frame.
	encode_u8(log_buffer, (delta_time << 7) | west_team->current_playtype());
	encode_u16(log_buffer, static_cast<int16_t>(the_ball->position().x * 1000.0 + 0.49));
	encode_u16(log_buffer, static_cast<int16_t>(the_ball->position().y * 1000.0 + 0.49));
	team::ptr teams[2] = {west_team, east_team};
	for (unsigned int tidx = 0; tidx < 2; ++tidx) {
		encode_u8(log_buffer, teams[tidx]->size() | (teams[tidx]->yellow() ? (1 << 7) : 0) | (delta_scores[tidx] ? (1 << 6) : 0));
		for (std::size_t i = 0; i < teams[tidx]->size(); ++i) {
			robot::ptr bot = teams[tidx]->get_robot(i);
			encode_u16(log_buffer, static_cast<int16_t>(bot->position().x * 1000.0 + 0.49));
			encode_u16(log_buffer, static_cast<int16_t>(bot->position().y * 1000.0 + 0.49));
			encode_u16(log_buffer, static_cast<int16_t>(bot->orientation() * 10000.0 + 0.49));
		}
	}

	// Update the frame count.
	++frame_count;

	// If the buffer has reached the block size, flush it.
	if (log_buffer.size() >= BLOCK_SIZE) {
		flush();
	}
}

