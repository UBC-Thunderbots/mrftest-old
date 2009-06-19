#include "datapool/IntervalTimer.h"
#include "Log/Log.h"

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <sigc++/sigc++.h>
#include <glibmm.h>

IntervalTimer::IntervalTimer(unsigned long long interval) {
	// Create the timer.
	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (fd < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IntervalTimer") << "Failed to create timerfd: " << std::strerror(err) << '\n';
		std::exit(1);
	}

	// Set the period.
	itimerspec spec;
	spec.it_interval.tv_sec  = interval / 1000000000ULL;
	spec.it_interval.tv_nsec = interval % 1000000000ULL;
	spec.it_value = spec.it_interval;
	if (timerfd_settime(fd, 0, &spec, 0) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IntervalTimer") << "Failed to arm timerfd: " << std::strerror(err) << '\n';
		std::exit(1);
	}

	// Make the FD nonblocking.
	long flags = fcntl(fd, F_GETFL);
	if (flags < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IntervalTimer") << "Failed to get FD flags: " << std::strerror(err) << '\n';
		std::exit(1);
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IntervalTimer") << "Failed to set FD flags: " << std::strerror(err) << '\n';
		std::exit(1);
	}

	// Make Glib poll it.
	Glib::signal_io().connect(sigc::mem_fun(*this, &IntervalTimer::onIO), fd, Glib::IO_IN);
}

IntervalTimer::~IntervalTimer() {
	close(fd);
}

sigc::signal<void> &IntervalTimer::signal_expire() {
	return sig_expire;
}

bool IntervalTimer::onIO(Glib::IOCondition cond) {
	if (cond & (Glib::IO_ERR | Glib::IO_HUP | Glib::IO_NVAL)) {
		Log::log(Log::LEVEL_ERROR, "IntervalTimer") << "Error polling timerfd.\n";
		std::exit(1);
	}

	if (cond & Glib::IO_IN) {
		uint64_t count;
		ssize_t ret = read(fd, &count, sizeof(count));
		if (ret == sizeof(count)) {
			while (count--) {
				sig_expire();
			}
		} else if (ret == 0 || (ret < 0 && (errno == EWOULDBLOCK || errno == EAGAIN))) {
			// Do nothing.
		} else {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "IntervalTimer") << "Error reading timerfd: " << std::strerror(err) << '\n';
			std::exit(1);
		}
	}

	return true;
}

