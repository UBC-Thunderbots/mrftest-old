#include "util/clocksource_timerfd.h"
#include "util/exception.h"
#include <stdint.h>
#include <sys/timerfd.h>

namespace {
	FileDescriptor::Ptr create_timerfd(int clockid) {
		int fd = timerfd_create(clockid, 0);
		if (fd < 0) {
			throw SystemError("timerfd_create", errno);
		}
		return FileDescriptor::create_from_fd(fd);
	}
}

TimerFDClockSource::TimerFDClockSource(uint64_t interval) : tfd(create_timerfd(CLOCK_MONOTONIC)), nanoseconds(interval), overflow_message("Timer overflow!", Annunciator::Message::TriggerMode::EDGE) {
	tfd->set_blocking(false);
	itimerspec tspec;
	tspec.it_interval.tv_sec = nanoseconds / UINT64_C(1000000000);
	tspec.it_interval.tv_nsec = nanoseconds % UINT64_C(1000000000);
	tspec.it_value.tv_sec = 1;
	tspec.it_value.tv_nsec = 0;
	if (timerfd_settime(tfd->fd(), 0, &tspec, 0) < 0) {
		throw SystemError("timer_starttime", errno);
	}

	Glib::signal_io().connect(sigc::mem_fun(this, &TimerFDClockSource::on_readable), tfd->fd(), Glib::IO_IN);
}

bool TimerFDClockSource::on_readable(Glib::IOCondition) {
	uint64_t ticks;

	if (read(tfd->fd(), &ticks, sizeof(ticks)) != sizeof(ticks)) {
		throw SystemError("read(timerfd)", errno);
	}

	if (ticks > 1) {
		overflow_message.fire();
	}

	while (ticks--) {
		signal_tick.emit();
	}

	return true;
}

