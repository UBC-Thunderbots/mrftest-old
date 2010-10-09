#ifndef UTIL_CRC16_H
#define UTIL_CRC16_H

#include <cstddef>
#include <stdint.h>

namespace CRC16 {
	/**
	 * The initial value of a CRC16 before updating it with any bytes.
	 */
	const uint16_t INITIAL = UINT16_C(0xFFFF);

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

	/**
	 * Computes the CRC16 of a block of data.
	 *
	 * \param[in] data the data to checksum.
	 *
	 * \param[in] len the length, in bytes, of \p data.
	 *
	 * \param[in] crc the initial value, computed from prior data.
	 *
	 * \return the CRC16.
	 */
	uint16_t calculate(const void *data, std::size_t len, uint16_t crc) __attribute__((warn_unused_result));
}

#endif

