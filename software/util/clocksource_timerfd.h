#ifndef UTIL_CLOCKSOURCE_TIMERFD_H
#define UTIL_CLOCKSOURCE_TIMERFD_H

#include "util/clocksource.h"
#include "util/fd.h"
#include <stdint.h>
#include <glibmm.h>

//
// A clock source implemented using the Linux timerfd mechanism.
//
class clocksource_timerfd : public clocksource, public sigc::trackable {
	public:
		//
		// Constructs a new clocksource that fires at the specified interval.
		//
		clocksource_timerfd(uint64_t nanoseconds);

		//
		// Starts the clock source.
		//
		void start();

		//
		// Stops the clock source.
		//
		void stop();

	private:
		file_descriptor tfd;
		uint64_t nanoseconds;
		bool running;
		bool on_readable(Glib::IOCondition);
};

#endif

