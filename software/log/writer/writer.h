#ifndef LOG_WRITER_WRITER_H
#define LOG_WRITER_WRITER_H

#include "util/byref.h"
#include "util/clocksource.h"
#include "util/fd.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/playtype.h"
#include "world/team.h"
#include <vector>
#include <stdint.h>

//
// A class that manages writing log files of matches to disk.
//
class log_writer : public byref, public sigc::trackable {
	public:
		//
		// A pointer to a log_writer.
		//
		typedef Glib::RefPtr<log_writer> ptr;

		//
		// Constructs a new log_writer.
		//
		static ptr create() {
			ptr p(new log_writer);
			return p;
		}

		//
		// Writes a frame to the log file.
		//
		void write_frame(field::ptr the_field, ball::ptr the_ball, team::ptr west_team, team::ptr east_team);

	private:
		log_writer();
		~log_writer();

		file_descriptor log_file;
		file_descriptor index_file;
		std::vector<uint8_t> log_buffer;
		uint64_t frame_count, byte_count;
		int16_t last_field_length, last_field_total_length, last_field_width, last_field_total_width, last_field_goal_width, last_field_centre_circle_radius, last_field_defense_area_radius, last_field_defense_area_stretch;
		uint32_t last_score_west, last_score_east;

		void flush();
};

#endif

