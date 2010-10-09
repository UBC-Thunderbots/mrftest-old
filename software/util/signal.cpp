#include "util/exception.h"
#include "util/signal.h"
#include <cerrno>

SignalHandlerScopedRegistration::SignalHandlerScopedRegistration(int sig, SignalHandlerFunction handler, int flags) : sig(sig) {
	struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = flags;
	if (sigaction(sig, &sa, &old) < 0) {
		throw SystemError("sigaction", errno);
	}
}

SignalHandlerScopedRegistration::~SignalHandlerScopedRegistration() {
	sigaction(sig, &old, 0);
}

SignalStackScopedRegistration::SignalStackScopedRegistration(void *stk, std::size_t len) {
	stack_t s;
	s.ss_sp = stk;
	s.ss_flags = 0;
	s.ss_size = len;
	if (sigaltstack(&s, &old) < 0) {
		throw SystemError("sigaltstack", errno);
	}
}

SignalStackScopedRegistration::~SignalStackScopedRegistration() {
	sigaltstack(&old, 0);
}

