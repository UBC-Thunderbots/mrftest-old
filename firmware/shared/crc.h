#ifndef SHARED_CRC_H
#define SHARED_CRC_H

/**
 * \file
 *
 * \brief Provides capabilities for computing CRC16s of data.
 */

#include <stdint.h>

/**
 * \brief The CRC16 of zero bytes.
 */
#define CRC16_EMPTY 0xFFFF

/**
 * \brief Updates a CRC with a byte.
 *
 * \param[in] crc the CRC to update.
 *
 * \param[in] ch the byte to update with.
 *
 * \return the new CRC.
 */
uint16_t crc_update(uint16_t crc, uint8_t ch);

/**
 * \brief Updates a CRC with a block of bytes.
 *
 * \param[in] crc the CRC to update.
 *
 * \param[in] pch the first byte to update with.
 *
 * \param[in] len the number of bytes to update with.
 *
 * \return the new CRC.
 */
uint16_t crc_update_block(uint16_t crc, __data const void *pch, uint8_t len);

#endif

