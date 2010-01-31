#ifndef LOG_READER_ROBOT_H
#define LOG_READER_ROBOT_H

#include "world/robot_impl.h"
#include <glibmm.h>

class log_reader;

//
// A robot whose position and orientation are driven by a replay of a log file.
//
class log_reader_robot : public robot_impl {
	public:
		//
		// A pointer to a log_reader_robot.
		//
		typedef Glib::RefPtr<log_reader_robot> ptr;

		//
		// Creates a new log_reader_robot.
		//
		static ptr create(log_reader &reader) {
			ptr p(new log_reader_robot(reader));
			return p;
		}

		//
		// Reads a robot structure from the log file.
		//
		void update();

		point position() const {
			return pos;
		}

		double orientation() const {
			return ori;
		}

		void ext_drag(const point &, const point &) {
		}

		void ext_rotate(double, double) {
		}

	private:
		log_reader &reader;
		point pos;
		double ori;

		log_reader_robot(log_reader &reader);
};

#endif

