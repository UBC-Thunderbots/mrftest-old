#ifndef XBEE_DAEMON_FRONTEND_ALREADY_RUNNING_H
#define XBEE_DAEMON_FRONTEND_ALREADY_RUNNING_H

#include <stdexcept>

/**
 * This exception is thrown if an attempt is made to construct a XBeeDaemon object while an arbiter is already running.
 * This situation is detected by the lock file being locked.
 */
class AlreadyRunning : public std::runtime_error {
	public:
		/**
		 * Constructs a new AlreadyRunning.
		 */
		AlreadyRunning();

		/**
		 * Destroys the AlreadyRunning.
		 */
		~AlreadyRunning() throw ();
};

#endif

