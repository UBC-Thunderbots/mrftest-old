#ifndef LOG_READER_BALL_H
#define LOG_READER_BALL_H

#include "geom/point.h"
#include "world/ball_impl.h"
#include <glibmm.h>

class log_reader;

//
// A ball whose position is driven by a log file.
//
class log_reader_ball : public ball_impl {
	public:
		//
		// A pointer to a log_reader_ball.
		//
		typedef Glib::RefPtr<log_reader_ball> ptr;

		//
		// Creates a log_reader_ball.
		//
		static ptr create(log_reader &r) {
			ptr p(new log_reader_ball(r));
			return p;
		}

		//
		// Updates the ball's position from the log.
		//
		void update();

		point position() const {
			return pos;
		}

		void ext_drag(const point &, const point &) {
		}

	private:
		log_reader &reader;
		point pos;

		log_reader_ball(log_reader &r);
};

#endif

