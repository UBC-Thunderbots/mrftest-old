#ifndef UTIL_CLOCKSOURCE_QUICK_H
#define UTIL_CLOCKSOURCE_QUICK_H

#include "util/clocksource.h"

/**
 * A clock source that fires as quickly as possible.
 */
class QuickClockSource : public ClockSource, public sigc::trackable {
	public:
		/**
		 * Starts the ClockSource.
		 */
		void start();

		/**
		 * Stops the ClockSource.
		 */
		void stop();

	private:
		sigc::connection connection;
		bool on_idle();
};

#endif

