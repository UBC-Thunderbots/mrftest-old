#ifndef UTIL_CLOCKSOURCE_H
#define UTIL_CLOCKSOURCE_H

#include <sigc++/sigc++.h>

/**
 * A source of timer ticks. A clock source is initially stopped.
 */
class ClockSource {
	public:
		/**
		 * Fired on each clock tick.
		 */
		sigc::signal<void> signal_tick;
};

#endif

