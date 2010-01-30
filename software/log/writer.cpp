#define __STDC_CONSTANT_MACROS
#include "log/writer.h"
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <bzlib.h>



namespace {
	const std::size_t BLOCK_SIZE = 128 * 1024;
	const uint64_t BLOCK_SIGNATURE = UINT64_C(0x5485485915675249);

	std::string get_file_name(std::time_t stamp, const std::string &extension) {
		const std::string &base = Glib::get_user_data_dir();
		const std::string &sub = base + "/thunderbots";
		mkdir(sub.c_str(), 0777);
		std::tm *tim = std::localtime(&stamp);
		char timebuf[256];
		std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S %Z", tim);
		const std::string &file = base + '/' + timebuf + '.' + extension;
		return file;
	}

	std::string get_log_file_name(std::time_t stamp) {
		return get_file_name(stamp, "log");
	}

	std::string get_index_file_name(std::time_t stamp) {
		return get_file_name(stamp, "idx");
	}

	void encode_u8(std::vector<uint8_t> &v, uint8_t x) {
		v.push_back(x);
	}

	void encode_u16(std::vector<uint8_t> &v, uint16_t x) {
		encode_u8(v, x >> 8);
		encode_u8(v, x);
	}

	void encode_u32(std::vector<uint8_t> &v, uint32_t x) {
		encode_u16(v, x >> 16);
		encode_u16(v, x);
	}

	void encode_u64(std::vector<uint8_t> &v, uint64_t x) {
		encode_u32(v, x >> 32);
		encode_u32(v, x);
	}

	bool is_ok_delta_score(int d) {
		return d == 0 || d == 1;
	}
}



log_writer::log_writer(clocksource &clksrc, ball::ptr theball, team::ptr wteam, team::ptr eteam) : the_ball(theball), west_team(wteam), east_team(eteam), last_frame_time(std::time(0)), log_file(get_log_file_name(last_frame_time).c_str(), O_WRONLY | O_CREAT | O_EXCL), index_file(get_index_file_name(last_frame_time).c_str(), O_WRONLY | O_CREAT | O_EXCL), frame_count(0), byte_count(0), last_score_west(0), last_score_east(0) {
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

	// Write an index record.
	std::vector<uint8_t> index_record;
	encode_u64(index_record, frame_count);
	encode_u64(index_record, byte_count);
	ssz = write(index_file, &index_record[0], index_record.size());
	if (ssz != static_cast<ssize_t>(index_record.size())) {
		throw std::runtime_error("Error writing to log index file!");
	}

	// Update byte count to reflect newly written data.
	byte_count += bzoutlen;

	// Clear log buffer.
	log_buffer.clear();
}



void log_writer::tick() {
	// Take a timestamp.
	std::time_t now = std::time(0);
	std::time_t delta_time = now - last_frame_time;
	int delta_score_west = west_team->score() - last_score_west;
	int delta_score_east = east_team->score() - last_score_east;
	last_frame_time = now;
	last_score_west = west_team->score();
	last_score_east = east_team->score();

	// Each frame record looks like this:
	//  1 byte:
	//   1 bit:  delta timestamp (0 or 1)
	//   5 bits: play type
	//   1 bit:  delta west score (0 or 1)
	//   1 bit:  delta east score (0 or 1)
	//  1 byte: # robots on west team
	//  1 byte: # robots on east team
	//  2 bytes: ball X position
	//  2 bytes: ball Y position
	//  For each robot (west team first, then east team):
	//   2 bytes: X position
	//   2 bytes: Y position
	//   2 bytes: orientation
	//
	// Positions are measured in millimetres.
	// Orientations are measured in 10,000ths of radians.
	//
	// The first byte requires that we have no more than 32 play types. Check
	// this at compile time.
	typedef char ASSERTION_CHECK_PLAY_TYPE_COUNT[playtype::count > 32 ? -1 : 1];

	// We require, with no exceptions, that there be no more than 255 robots per team.
	if (west_team->size() > 255 || east_team->size() > 255) {
		throw std::runtime_error("Team too big to log!");
	}

	// We also require that deltas of time and score be no more than 1. If they
	// would be, flush the buffer to allow absolute values to be written in a
	// block header.
	if (delta_time > 1 || !is_ok_delta_score(delta_score_west) || !is_ok_delta_score(delta_score_east)) {
		flush();
		delta_time = 0;
		delta_score_west = 0;
		delta_score_east = 0;
	}

	// Write a header if needed.
	if (log_buffer.empty()) {
		last_frame_time = now;
		last_score_west = west_team->score();
		last_score_east = east_team->score();
		encode_u64(log_buffer, last_frame_time);
		encode_u64(log_buffer, frame_count);
		encode_u32(log_buffer, last_score_west);
		encode_u32(log_buffer, last_score_east);
	}

	// Write the data for this frame.
	encode_u8(log_buffer, (delta_time << 7) | (west_team->current_playtype() << 2) | (delta_score_west << 1) | delta_score_east);
	encode_u8(log_buffer, west_team->size());
	encode_u8(log_buffer, east_team->size());
	encode_u16(log_buffer, static_cast<int16_t>(the_ball->position().x * 1000.0));
	encode_u16(log_buffer, static_cast<int16_t>(the_ball->position().y * 1000.0));
	for (std::size_t i = 0; i < west_team->size(); ++i) {
		robot::ptr bot = west_team->get_robot(i);
		encode_u16(log_buffer, static_cast<int16_t>(bot->position().x * 1000.0));
		encode_u16(log_buffer, static_cast<int16_t>(bot->position().y * 1000.0));
		encode_u16(log_buffer, static_cast<int16_t>(bot->orientation() * 10000.0));
	}
	for (std::size_t i = 0; i < east_team->size(); ++i) {
		robot::ptr bot = east_team->get_robot(i);
		encode_u16(log_buffer, static_cast<int16_t>(bot->position().x * 1000.0));
		encode_u16(log_buffer, static_cast<int16_t>(bot->position().y * 1000.0));
		encode_u16(log_buffer, static_cast<int16_t>(bot->orientation() * 10000.0));
	}

	// If the buffer has reached the block size, flush it.
	if (log_buffer.size() >= BLOCK_SIZE) {
		flush();
	}
}

