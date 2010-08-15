#ifndef XBEE_DAEMON_FRONTEND_RESOURCE_ALLOCATION_FAILED_H
#define XBEE_DAEMON_FRONTEND_RESOURCE_ALLOCATION_FAILED_H

#include <exception>

/**
 * Thrown if an attempt is made to bring a robot into drive mode but sufficient
 * resources are not available to do so.
 */
class ResourceAllocationFailed : public std::exception {
	public:
		/**
		 * Returns a string message describing the situation.
		 *
		 * \return the message.
		 */
		const char *what() const throw() {
			return "Insufficient resources were available to enter drive mode.";
		}
};

#endif

