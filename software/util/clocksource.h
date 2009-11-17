#ifndef UTIL_CLOCKSOURCE_H
#define UTIL_CLOCKSOURCE_H

#include <sigc++/sigc++.h>

//
// A source of timer ticks. A clock source is initially stopped.
//
class clocksource {
	public:
		//
		// Returns a signal that is fired on each clock tick.
		//
		sigc::signal<void> &signal_tick() {
			return sig_tick;
		}

		//
		// Starts the clock.
		//
		virtual void start() = 0;

		//
		// Stops the clock.
		//
		virtual void stop() = 0;

	private:
		sigc::signal<void> sig_tick;
};

#endif

