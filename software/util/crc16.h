#ifndef UTIL_CRC16_H
#define UTIL_CRC16_H

#include <cstddef>
#include <stdint.h>

namespace CRC16 {
	/**
	 * Computes the CRC16 of a block of data.
	 *
	 * \param[in] data the data to checksum.
	 *
	 * \param[in] len the length, in bytes, of \p data.
	 *
	 * \return the CRC16.
	 */
	uint16_t calculate(const void *data, std::size_t len) __attribute__((warn_unused_result));
}

#endif

