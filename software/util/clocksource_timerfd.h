#ifndef UTIL_CLOCKSOURCE_TIMERFD_H
#define UTIL_CLOCKSOURCE_TIMERFD_H

#include "uicomponents/annunciator.h"
#include "util/clocksource.h"
#include "util/fd.h"
#include <glibmm.h>
#include <stdint.h>

/**
 * A clock source implemented using the Linux timerfd mechanism.
 */
class TimerFDClockSource : public ClockSource, public sigc::trackable {
	public:
		/**
		 * Constructs a new TimerFDClockSource that fires at the specified interval.
		 *
		 * \param[in] nanoseconds the number of nanoseconds between consecutive firings of the timer.
		 */
		TimerFDClockSource(uint64_t nanoseconds);

	private:
		const FileDescriptor::Ptr tfd;
		const uint64_t nanoseconds;
		Annunciator::Message overflow_message;
		bool on_readable(Glib::IOCondition);
};

#endif

