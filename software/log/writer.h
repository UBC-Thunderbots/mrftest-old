#ifndef LOG_WRITER_H
#define LOG_WRITER_H

#include "util/byref.h"
#include "util/clocksource.h"
#include "util/fd.h"
#include "world/ball.h"
#include "world/playtype.h"
#include "world/team.h"
#include <vector>
#include <ctime>
#include <stdint.h>

//
// A class that manages writing log files of matches to disk.
//
class log_writer : public byref, public sigc::trackable {
	public:
		//
		// Constructs a new log_writer. The log_writer will begin writing to the
		// log immediately at the next tick.
		//
		log_writer(clocksource &clksrc, ball::ptr theball, team::ptr wteam, team::ptr eteam);

		//
		// Destroys a log_writer. The file is flushed if necessary.
		// 
		~log_writer();

	private:
		ball::ptr the_ball;
		team::ptr west_team, east_team;

		std::time_t last_frame_time;
		file_descriptor log_file;
		file_descriptor index_file;
		std::vector<uint8_t> log_buffer;
		uint64_t frame_count, byte_count;
		uint32_t last_score_west, last_score_east;

		void flush();
		void tick();
};

#endif

