#ifndef UTIL_CLOCKSOURCE_H
#define UTIL_CLOCKSOURCE_H

#include <sigc++/sigc++.h>

//
// A source of timer ticks.
//
class clocksource {
	public:
		//
		// Returns a signal that is fired on each clock tick.
		//
		sigc::signal<void> &signal_tick() {
			return sig_tick;
		}

	private:
		sigc::signal<void> sig_tick;
};

#endif

