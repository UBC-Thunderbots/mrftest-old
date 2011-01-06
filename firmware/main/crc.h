#ifndef CRC_H
#define CRC_H

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

#endif

