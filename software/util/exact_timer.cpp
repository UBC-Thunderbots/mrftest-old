#include "util/exact_timer.h"
#include "world/timestep.h"
#include <time.h>
#include <sigc++/sigc++.h>
#include <glibmm.h>

#ifndef CLOCK_MONOTONIC
	#define CLOCK_MONOTONIC CLOCK_REALTIME
#endif

class exact_timer_impl : public virtual sigc::trackable {
	public:
		exact_timer_impl(exact_timer &tmr, double interval) : interval(interval), tmr(tmr), invocations(0) {
			// Remember start time.
			clock_gettime(CLOCK_MONOTONIC, &start_time);

			// Round the interval *UP* to the nearest millisecond. It must be up, because it's easier to do
			// multiple invocations of the callback in a single timer expiration to catch up with slow ticks
			// than it is to skip invoking the callback sometimes without becoming jittery.
			unsigned int millis = static_cast<unsigned int>((interval * 1000.0) + 0.9999999);

			// Connect a timeout signal.
			conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &exact_timer_impl::fired), millis);
		}

	private:
		bool fired() {
			// Get the time difference from the start time.
			timespec diff;
			clock_gettime(CLOCK_MONOTONIC, &diff);
			diff.tv_sec -= start_time.tv_sec;
			diff.tv_nsec -= start_time.tv_nsec;
			if (diff.tv_nsec < 0) {
				diff.tv_sec--;
				diff.tv_nsec += 1000000000L;
			}

			// Compute how many invocations that ought to be.
			double diff_dbl = diff.tv_sec + diff.tv_nsec / 1000000000.0;
			unsigned int desired_invocations = static_cast<unsigned int>(diff_dbl / interval);

			// Invoke the callback the necessary number of times.
			while (invocations < desired_invocations) {
				tmr.signal_expired().emit();
				invocations++;
			}
			return true;
		}

		double interval;
		exact_timer &tmr;
		timespec start_time;
		unsigned int invocations;
		sigc::connection conn;
};

exact_timer::exact_timer(double interval) : impl(new exact_timer_impl(*this, interval)) {
}

exact_timer::~exact_timer() {
	delete impl;
}

