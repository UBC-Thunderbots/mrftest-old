#ifndef UTIL_CLOCKSOURCE_TIMERFD_H
#define UTIL_CLOCKSOURCE_TIMERFD_H

#include "uicomponents/annunciator.h"
#include "util/clocksource.h"
#include "util/fd.h"
#include <stdint.h>
#include <glibmm.h>

/**
 * A clock source implemented using the Linux timerfd mechanism.
 */
class TimerFDClockSource : public ClockSource, public sigc::trackable {
	public:
		/**
		 * Constructs a new TimerFDClockSource that fires at the specified
		 * interval.
		 *
		 * \param[in] nanoseconds the number of nanoseconds between consecutive
		 * firings of the timer.
		 */
		TimerFDClockSource(uint64_t nanoseconds);

		/**
		 * Starts the clock source.
		 */
		void start();

		/**
		 * Stops the clock source.
		 */
		void stop();

	private:
		const FileDescriptor::Ptr tfd;
		const uint64_t nanoseconds;
		bool running;
		Annunciator::message overflow_message;
		bool on_readable(Glib::IOCondition);
};

#endif

