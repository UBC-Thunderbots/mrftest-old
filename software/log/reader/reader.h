#ifndef LOG_READER_READER_H
#define LOG_READER_READER_H

#include "log/reader/ball.h"
#include "log/reader/field.h"
#include "log/reader/index.h"
#include "log/reader/logfile.h"
#include "log/reader/team.h"
#include "util/byref.h"
#include "util/fd.h"
#include "world/ball.h"
#include <string>
#include <utility>
#include <vector>
#include <cstddef>
#include <ctime>
#include <stdint.h>

//
// Allows reading from a recorded log file.
//
class log_reader : public byref {
	public:
		//
		// A pointer to a log_reader.
		//
		typedef Glib::RefPtr<log_reader> ptr;

		//
		// Gets a list of all recorded logs.
		//
		static std::vector<std::string> all_logs();

		//
		// Opens a log.
		//
		ptr create(const std::string &name) {
			ptr p(new log_reader(name));
			return p;
		}

		//
		// Returns the number of frames in the log.
		//
		uint64_t size() const {
			return the_size;
		}

		//
		// Returns the current frame.
		//
		uint64_t tell() const {
			return current_frame;
		}

		//
		// Moves to a specified frame.
		//
		void seek(uint64_t frame);

		//
		// Advances forward by one frame.
		//
		void next_frame();

		//
		// Reads a uint8_t from the log.
		//
		uint8_t read_u8();

		//
		// Reads a uint16_t from the log.
		//
		uint16_t read_u16();

		//
		// Reads a uint32_t from the log.
		//
		uint32_t read_u32();

		//
		// Reads a uint64_t from the log.
		//
		uint64_t read_u64();

	private:
		log_reader_logfile log_file;
		log_reader_index index;
		uint64_t the_size;
		uint64_t current_frame;
		uint64_t current_block_address;
		std::size_t current_block_offset;
		log_reader_field::ptr fld;
		log_reader_ball::ptr the_ball_impl;
		ball::ptr the_ball;
		log_reader_team::ptr west_team, east_team;
		std::time_t frame_time;

		log_reader(const std::string &name);

		~log_reader() {
		}
};

#endif

