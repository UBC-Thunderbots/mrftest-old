#ifndef XBEE_DAEMON_FRONTEND_RESOURCE_ALLOCATION_FAILED_H
#define XBEE_DAEMON_FRONTEND_RESOURCE_ALLOCATION_FAILED_H

#include <stdexcept>

/**
 * Thrown if an attempt is made to bring a robot into drive mode but sufficient resources are not available to do so.
 */
class ResourceAllocationFailed : public std::runtime_error {
	public:
		/**
		 * Constructs a new ResourceAllocationFailed.
		 */
		ResourceAllocationFailed();

		/**
		 * Destroys the ResourceAllocationFailed.
		 */
		~ResourceAllocationFailed() throw ();
};

#endif

