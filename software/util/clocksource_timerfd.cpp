#include "util/clocksource_timerfd.h"
#include <stdexcept>
#include <sys/timerfd.h>

namespace {
	file_descriptor create_timerfd(int clockid) {
		int fd = timerfd_create(clockid, 0);
		if (fd < 0)
			throw std::runtime_error("Cannot create timerfd!");
		return file_descriptor::create(fd);
	}
}

clocksource_timerfd::clocksource_timerfd(uint64_t interval) : tfd(create_timerfd(CLOCK_MONOTONIC)), nanoseconds(interval), running(false) {
	tfd.set_blocking(false);

	Glib::signal_io().connect(sigc::mem_fun(*this, &clocksource_timerfd::on_readable), tfd, Glib::IO_IN);
}

bool clocksource_timerfd::on_readable(Glib::IOCondition) {
	uint64_t ticks;

	if (read(tfd, &ticks, sizeof(ticks)) != sizeof(ticks))
		throw std::runtime_error("Cannot read timerfd!");

	while (ticks-- && running)
		signal_tick().emit();

	return true;
}

void clocksource_timerfd::start() {
	itimerspec tspec;
	tspec.it_interval.tv_sec  = nanoseconds / 1000000000ULL;
	tspec.it_interval.tv_nsec = nanoseconds % 1000000000ULL;
	tspec.it_value = tspec.it_interval;
	if (timerfd_settime(tfd, 0, &tspec, 0) < 0)
		throw std::runtime_error("Cannot start timerfd!");
	running = true;
}

void clocksource_timerfd::stop() {
	itimerspec tspec;
	tspec.it_interval.tv_sec  = 0;
	tspec.it_interval.tv_nsec = 0;
	tspec.it_value = tspec.it_interval;
	if (timerfd_settime(tfd, 0, &tspec, 0) < 0)
		throw std::runtime_error("Cannot stop timerfd!");
	running = false;
}

