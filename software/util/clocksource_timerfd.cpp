#include "util/clocksource_timerfd.h"
#include <stdexcept>
#include <stdint.h>
#include <sys/timerfd.h>

namespace {
	FileDescriptor create_timerfd(int clockid) {
		int fd = timerfd_create(clockid, 0);
		if (fd < 0)
			throw std::runtime_error("Cannot create timerfd!");
		return FileDescriptor::create(fd);
	}
}

TimerFDClockSource::TimerFDClockSource(uint64_t interval) : tfd(create_timerfd(CLOCK_MONOTONIC)), nanoseconds(interval), running(false), overflow_message("Timer overflow!") {
	tfd.set_blocking(false);

	Glib::signal_io().connect(sigc::mem_fun(this, &TimerFDClockSource::on_readable), tfd, Glib::IO_IN);
}

bool TimerFDClockSource::on_readable(Glib::IOCondition) {
	uint64_t ticks;

	if (read(tfd, &ticks, sizeof(ticks)) != sizeof(ticks))
		throw std::runtime_error("Cannot read timerfd!");

	overflow_message.activate(ticks > 1);

	while (ticks-- && running)
		signal_tick.emit();

	return true;
}

void TimerFDClockSource::start() {
	itimerspec tspec;
	tspec.it_interval.tv_sec  = nanoseconds / UINT64_C(1000000000);
	tspec.it_interval.tv_nsec = nanoseconds % UINT64_C(1000000000);
	tspec.it_value = tspec.it_interval;
	if (timerfd_settime(tfd, 0, &tspec, 0) < 0)
		throw std::runtime_error("Cannot start timerfd!");
	running = true;
}

void TimerFDClockSource::stop() {
	itimerspec tspec;
	tspec.it_interval.tv_sec  = 0;
	tspec.it_interval.tv_nsec = 0;
	tspec.it_value = tspec.it_interval;
	if (timerfd_settime(tfd, 0, &tspec, 0) < 0)
		throw std::runtime_error("Cannot stop timerfd!");
	running = false;
}

