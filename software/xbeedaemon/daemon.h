#ifndef XBEEDAEMON_DAEMON_H
#define XBEEDAEMON_DAEMON_H

#include "util/fd.h"

namespace xbeedaemon {
	//
	// Tries to launch the XBee daemon.
	//
	// Parameters:
	//  sock
	//   A reference to a file_descriptor object.
	//   On success, receives a socket connected to the daemon.
	//   On failure, not modified.
	//
	// Return value:
	//  true
	//   If the daemon was successfully launched.
	//
	//  false
	//   If the lock file could not be locked, meaning a daemon is already
	//   running.
	//
	bool launch(file_descriptor &sock) __attribute__((warn_unused_result));
};

#endif

