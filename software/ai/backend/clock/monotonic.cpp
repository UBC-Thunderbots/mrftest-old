#include "ai/backend/clock/monotonic.h"
#include "util/exception.h"
#include "util/timestep.h"
#include <cerrno>
#include <cstdint>
#include <unistd.h>
#include <sys/timerfd.h>

using AI::BE::Clock::Monotonic;

namespace {
	FileDescriptor create_timerfd(int clockid) {
		int fd = timerfd_create(clockid, 0);
		if (fd < 0) {
			throw SystemError("timerfd_create", errno);
		}
		return FileDescriptor::create_from_fd(fd);
	}
}

Monotonic::Monotonic() : tfd(create_timerfd(CLOCK_MONOTONIC)), overflow_message(u8"Timer overflow!", Annunciator::Message::TriggerMode::EDGE, Annunciator::Message::Severity::LOW) {
	const uint64_t nanoseconds = (UINT64_C(1000000000) + TIMESTEPS_PER_SECOND / 2) / TIMESTEPS_PER_SECOND;
	tfd.set_blocking(false);
	itimerspec tspec;
	tspec.it_interval.tv_sec = static_cast<time_t>(nanoseconds / UINT64_C(1000000000));
	tspec.it_interval.tv_nsec = static_cast<long>(nanoseconds % UINT64_C(1000000000));
	tspec.it_value.tv_sec = 1;
	tspec.it_value.tv_nsec = 0;
	if (timerfd_settime(tfd.fd(), 0, &tspec, nullptr) < 0) {
		throw SystemError("timerfd_settime", errno);
	}

	Glib::signal_io().connect(sigc::mem_fun(this, &Monotonic::on_readable), tfd.fd(), Glib::IO_IN);
}

bool Monotonic::on_readable(Glib::IOCondition) {
	uint64_t ticks;

	if (read(tfd.fd(), &ticks, sizeof(ticks)) != sizeof(ticks)) {
		throw SystemError("read(timerfd)", errno);
	}

	if (ticks > 1) {
		overflow_message.fire();
	}

	signal_tick.emit();

	return true;
}

