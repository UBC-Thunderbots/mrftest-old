#ifndef UTIL_XBEE_H
#define UTIL_XBEE_H

#include <stdint.h>

/**
 * A collection of utility functions for XBee.
 */
namespace XBeeUtil {
	/**
	 * Converts a bot address to an array of bytes.
	 *
	 * \param[in] src the address to convert.
	 *
	 * \param[out] dest the location at which to store the encoded address.
	 */
	void address_to_bytes(uint64_t src, uint8_t *dest);

	/**
	 * Extracts a bot address from an array of bytes.
	 *
	 * \param[in] src the bytes from which to extract the address.
	 *
	 * \return the decoded address.
	 */
	uint64_t address_from_bytes(const uint8_t *src) __attribute__((warn_unused_result));
}

#endif

