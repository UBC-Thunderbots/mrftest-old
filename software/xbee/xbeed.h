#ifndef XBEE_XBEED_PROTO_H
#define XBEE_XBEED_PROTO_H

#include "util/fd.h"
#include <stdint.h>

namespace xbeed {
	//
	// Request types.
	//
	
	// Requests that the daemon verify its existence.
	// The daemon responds with RESPONSE_PONG.
	const uint8_t REQUEST_PING = 0x01;

	// Requests that the daemon grant the lock to the caller.
	// The daemon responds with RESPONSE_GRANT.
	// This request has a high priority.
	const uint8_t REQUEST_LOCK_INTERACTIVE = 0x02;

	// Requests that the daemon grant the lock to the caller.
	// The daemon responds with ESPONSE_GRANT.
	// This request has a low priority.
	const uint8_t REQUEST_LOCK_BATCH = 0x03;

	// Requests that the daemon remove the lock from the caller.
	// The daemon does not respond.
	const uint8_t REQUEST_UNLOCK = 0x04;

	//
	// Response types.
	//
	
	// Indicates that the daemon is alive.
	const uint8_t RESPONSE_PONG = 0x81;

	// Indicates that the recipient has been granted the lock.
	const uint8_t RESPONSE_GRANT = 0x82;

	//
	// Tries launching the daemon.
	// Returns true if we launched the daemon.
	// Returns false if someone else got there first.
	// If we launched the daemon, a connected socket is passed out.
	// You must pass the file descriptor of the lock file.
	//
	bool launch(const file_descriptor &lock_fd, file_descriptor &client_sock);
}

#endif

