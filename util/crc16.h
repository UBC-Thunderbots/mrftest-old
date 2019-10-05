#ifndef UTIL_CRC16_H
#define UTIL_CRC16_H

#include <cstddef>
#include <cstdint>

namespace CRC16
{
/**
 * The initial value of a CRC16 before updating it with any bytes.
 */
const uint16_t INITIAL = UINT16_C(0xFFFF);

/**
 * Computes the CRC16 of a byte of data.
 *
 * \param[in] crc the initial value, computed from prior data.
 *
 * \param[in] data the data to checksum.
 *
 * \return the CRC16.
 */
uint16_t calculate(uint16_t crc, uint8_t data)
    __attribute__((warn_unused_result));

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
uint16_t calculate(const void *data, std::size_t len, uint16_t crc = INITIAL)
    __attribute__((warn_unused_result));
}

#endif
