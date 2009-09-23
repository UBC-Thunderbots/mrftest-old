#ifndef UTIL_EXACT_TIMER_H
#define UTIL_EXACT_TIMER_H

#include "util/mutex.h"
#include "util/noncopyable.h"
#include <sigc++/sigc++.h>

class exact_timer_impl;

//
// A timer that provides exact interval timing, irrespective of the time taken
// to handle each event.
//
class exact_timer : public virtual noncopyable {
	public:
		//
		// Constructs a new exact_timer to fire at the specified interval.
		//
		exact_timer(double interval);

		//
		// Destroys the timer.
		//
		~exact_timer();

		//
		// Returns the expiry signal.
		//
		sigc::signal<void> &signal_expired() {
			return the_signal_expired;
		}

	private:
		sigc::signal<void> the_signal_expired;
		exact_timer_impl *impl;
};

#endif

