#include "util/signal.h"
#include <cerrno>
#include "util/exception.h"

SignalHandlerScopedRegistration::SignalHandlerScopedRegistration(
    int sig, SignalHandlerFunction handler, unsigned int flags)
    : sig(sig)
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = static_cast<int>(flags);
    if (sigaction(sig, &sa, &old) < 0)
    {
        throw SystemError("sigaction", errno);
    }
}

SignalHandlerScopedRegistration::~SignalHandlerScopedRegistration()
{
    sigaction(sig, &old, nullptr);
}

SignalStackScopedRegistration::SignalStackScopedRegistration(
    void *stk, std::size_t len)
{
    stack_t s;
    s.ss_sp    = stk;
    s.ss_flags = 0;
    s.ss_size  = len;
    if (sigaltstack(&s, &old) < 0)
    {
        throw SystemError("sigaltstack", errno);
    }
}

SignalStackScopedRegistration::~SignalStackScopedRegistration()
{
    sigaltstack(&old, nullptr);
}
