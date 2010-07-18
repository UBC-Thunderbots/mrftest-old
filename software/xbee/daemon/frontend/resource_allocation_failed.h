#ifndef XBEE_DAEMON_FRONTEND_RESOURCE_ALLOCATION_FAILED_H
#define XBEE_DAEMON_FRONTEND_RESOURCE_ALLOCATION_FAILED_H

#include <exception>

//
// This exception is thrown if an attempt is made to bring a robot into drive
// mode but sufficient resources are not available to do so.
//
class ResourceAllocationFailed : public std::exception {
	public:
		//
		// Returns a string message describing the situation.
		//
		const char *what() const throw() {
			return "Insufficient resources were available to enter drive mode.";
		}
};

#endif

