#include "log/reader/reader.h"
#include "util/codec.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <glibmm.h>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>



namespace {
	class filename_comparator {
		public:
			bool operator()(const std::string &x, const std::string &y) {
				const Glib::ustring &dx = Glib::filename_display_name(x);
				const Glib::ustring &dy = Glib::filename_display_name(y);
				return dx < dy;
			}
	};
}



std::vector<std::string> log_reader::all_logs() {
	// Compute the directory where logs would be stored.
	const std::string &tbdir = Glib::get_user_data_dir() + "/thunderbots";

	// Try to open the directory.
	try {
		Glib::Dir dir(tbdir);
		std::string elem;
		std::vector<std::string> vec;
		while (!(elem = dir.read_name()).empty()) {
			if (elem.size() >= 4 && elem[elem.size() - 4] == '.' && elem[elem.size() - 3] == 'l' && elem[elem.size() - 2] == 'o' && elem[elem.size() - 1] == 'g') {
				vec.push_back(elem.substr(0, elem.size() - 4));
			}
		}
		std::sort(vec.begin(), vec.end(), filename_comparator());
		return vec;
	} catch (const Glib::FileError &err) {
		if (err.code() == Glib::FileError::NO_SUCH_ENTITY) {
			// Directory not found. Just say there are no logs; it's not
			// strictly an error.
			return std::vector<std::string>();
		} else {
			throw;
		}
	}
}



log_reader::log_reader(const std::string &name) : log_file(name), index(name), the_size(index.size()), current_frame(0), fld(log_reader_field::create(*this)), the_ball_impl(log_reader_ball::create(*this)), the_ball(new ball(the_ball_impl, false)), west_team(log_reader_team::create(*this, false)), east_team(log_reader_team::create(*this, true)) {
	seek(0);
}



void log_reader::seek(uint64_t frame) {
	log_reader_index::record_type index_record = index.find_frame(frame);
	current_block_address = index_record.second;
	current_block_offset = 0;
	current_frame = index_record.first - 1;
	while (current_frame != frame) {
		next_frame();
	}
}



void log_reader::next_frame() {
	if (!current_block_offset) {
		frame_time = read_u64();
		uint64_t frame_count = read_u64();
		assert(frame_count == current_frame + 1);
		west_team->the_score = read_u32();
		east_team->the_score = read_u32();
		fld->update();
	}

	uint8_t u8 = read_u8();
	if (u8 & 0x80) {
		++frame_time;
	}
	west_team->the_playtype = static_cast<playtype::playtype>(u8 & 0x7F);
	east_team->the_playtype = playtype::invert[u8 & 0x7F];
	the_ball_impl->update();
	west_team->update();
	east_team->update();

	++current_frame;
}



uint8_t log_reader::read_u8() {
	const log_reader_logfile::block_type &block = log_file.load_block(current_block_address);
	return decode_u8(block.second, current_block_offset);
}

uint16_t log_reader::read_u16() {
	const log_reader_logfile::block_type &block = log_file.load_block(current_block_address);
	return decode_u16(block.second, current_block_offset);
}

uint32_t log_reader::read_u32() {
	const log_reader_logfile::block_type &block = log_file.load_block(current_block_address);
	return decode_u32(block.second, current_block_offset);
}

uint64_t log_reader::read_u64() {
	const log_reader_logfile::block_type &block = log_file.load_block(current_block_address);
	return decode_u64(block.second, current_block_offset);
}

