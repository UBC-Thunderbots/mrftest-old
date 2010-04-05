#ifndef UTIL_XBEE_H
#define UTIL_XBEE_H

#include <stdint.h>

//
// A collection of utility functions for XBee.
//
namespace xbeeutil {
	//
	// Converts a bot address from a uint64_t to an array of bytes.
	//
	void address_to_bytes(uint64_t src, uint8_t *dest);

	//
	// Converts a bot address from an array of bytes to a uint64_t.
	//
	uint64_t address_from_bytes(const uint8_t *src) __attribute__((warn_unused_result));
}

#endif

