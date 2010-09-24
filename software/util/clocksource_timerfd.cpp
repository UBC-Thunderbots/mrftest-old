#include "util/clocksource_timerfd.h"
#include <stdexcept>
#include <stdint.h>
#include <sys/timerfd.h>

namespace {
	FileDescriptor::Ptr create_timerfd(int clockid) {
		int fd = timerfd_create(clockid, 0);
		if (fd < 0) {
			throw std::runtime_error("Cannot create timerfd!");
		}
		return FileDescriptor::create_from_fd(fd);
	}
}

TimerFDClockSource::TimerFDClockSource(uint64_t interval) : tfd(create_timerfd(CLOCK_MONOTONIC)), nanoseconds(interval), overflow_message("Timer overflow!") {
	tfd->set_blocking(false);
	itimerspec tspec;
	tspec.it_interval.tv_sec  = nanoseconds / UINT64_C(1000000000);
	tspec.it_interval.tv_nsec = nanoseconds % UINT64_C(1000000000);
	tspec.it_value = tspec.it_interval;
	if (timerfd_settime(tfd->fd(), 0, &tspec, 0) < 0) {
		throw std::runtime_error("Cannot start timerfd!");
	}

	Glib::signal_io().connect(sigc::mem_fun(this, &TimerFDClockSource::on_readable), tfd->fd(), Glib::IO_IN);
}

bool TimerFDClockSource::on_readable(Glib::IOCondition) {
	uint64_t ticks;

	if (read(tfd->fd(), &ticks, sizeof(ticks)) != sizeof(ticks)) {
		throw std::runtime_error("Cannot read timerfd!");
	}

	overflow_message.activate(ticks > 1);

	while (ticks--) {
		signal_tick.emit();
	}

	return true;
}

