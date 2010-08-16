#include "xbee/daemon/frontend/resource_allocation_failed.h"

ResourceAllocationFailed::ResourceAllocationFailed() : std::runtime_error("Insufficient resources were available to enter drive mode.") {
}

ResourceAllocationFailed::~ResourceAllocationFailed() throw () {
}

