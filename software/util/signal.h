#ifndef UTIL_SIGNAL_H
#define UTIL_SIGNAL_H

#include <csignal>

/**
 * The type of a signal handler function.
 */
extern "C" typedef void (*SignalHandlerFunction)(int);

/**
 * Registers a signal handler for the scope of an object.
 */
class SignalHandlerScopedRegistration {
	public:
		/**
		 * Registers a signal handler.
		 *
		 * \param[in] sig the signal.
		 *
		 * \param[in] handler the signal handler to register.
		 *
		 * \param[in] flags the flags to use while registering the handler.
		 */
		SignalHandlerScopedRegistration(int sig, SignalHandlerFunction handler, int flags);

		/**
		 * Unregisters the signal handler.
		 */
		~SignalHandlerScopedRegistration();

	private:
		int sig;
		struct sigaction old;
};

/**
 * Registers an alternate stack for signal handling for the scope of an object.
 */
class SignalStackScopedRegistration {
	public:
		/**
		 * Registers an alternate stack.
		 *
		 * \param[in] stk the stack to register.
		 *
		 * \param[in] len the size of the stack.
		 */
		SignalStackScopedRegistration(void *stk, std::size_t len);

		/**
		 * Unregisters the alternate stack.
		 */
		~SignalStackScopedRegistration();

	private:
		stack_t old;
};

#endif

