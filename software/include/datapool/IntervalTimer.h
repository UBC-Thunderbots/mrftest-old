#ifndef DATAPOOL_INTERVALTIMER_H
#define DATAPOOL_INTERVALTIMER_H

#include "datapool/Noncopyable.h"

#include <sigc++/sigc++.h>
#include <glibmm.h>

//
// A timeout that fires at a fixed interval.
// This is in contrast to Glib's timeout signal, which waits a fixed amount of time after the last firing finishes.
//
class IntervalTimer : public virtual sigc::trackable, private virtual Noncopyable {
public:
	//
	// Creates a new interval timer with the specified interval, in nanoseconds.
	//
	IntervalTimer(unsigned long long interval);

	//
	// Destroys the interval timer.
	//
	~IntervalTimer();

	//
	// The signal that is fired at the interval.
	//
	sigc::signal<void> &signal_expire();

private:
	int fd;
	sigc::signal<void> sig_expire;

	bool onIO(Glib::IOCondition cond);
};

#endif

