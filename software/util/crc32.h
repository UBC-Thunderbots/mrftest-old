#ifndef UTIL_CRC32_H
#define UTIL_CRC32_H

#include <cstddef>
#include <cstdint>

namespace CRC32 {
	/**
	 * The initial value of a CRC32 before updating it with any bytes.
	 */
	const uint32_t INITIAL = UINT32_C(0xFFFFFFFF);

	/**
	 * Computes the CRC32 of a byte of data.
	 *
	 * \param[in] crc the initial value, computed from prior data.
	 *
	 * \param[in] data the data to checksum.
	 *
	 * \return the CRC32.
	 */
	uint32_t calculate(uint32_t crc, uint8_t data) __attribute__((warn_unused_result));

	/**
	 * Computes the CRC32 of a block of data.
	 *
	 * \param[in] data the data to checksum.
	 *
	 * \param[in] len the length, in bytes, of \p data.
	 *
	 * \param[in] crc the initial value, computed from prior data.
	 *
	 * \return the CRC32.
	 */
	uint32_t calculate(const void *data, std::size_t len, uint32_t crc = INITIAL) __attribute__((warn_unused_result));
}

#endif
