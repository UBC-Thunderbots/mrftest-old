#ifndef FLASH_H
#define FLASH_H

/**
 * \file
 *
 * \brief Provides the ability to send and receive data over SPI to the flash memory
 */

#include "io.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * \brief Starts a chip erase
 *
 * This function returns immediately.
 */
void flash_start_chip_erase(void);

/**
 * \brief Checks if the flash is busy
 *
 * \return \c true if the flash is busy, or \c false if not
 */
bool flash_is_busy(void);

/**
 * \brief Erases a 4 kB sector
 *
 * This function returns when the erase operation is complete.
 *
 * \param[in] address the address of the first byte in the sector
 */
void flash_erase_sector(uint32_t address);

/**
 * \brief Executes a page program
 *
 * This function returns when the program operation is complete.
 *
 * \param[in] page the page number to program
 *
 * \param[in] data the data to program
 *
 * \param[in] length the number of bytes of data, or 0 to program 256 bytes
 */
void flash_page_program(uint16_t page, const void *data, uint8_t length);

/**
 * \brief Reads a block of data from flash
 *
 * \param[in] start the address of the first byte to read
 *
 * \param[out] buffer the buffer to write into
 *
 * \param[in] length the number of bytes to read
 */
void flash_read(uint32_t start, void *buffer, uint8_t length);

/**
 * \brief Computes the sum of a range of bytes
 *
 * \param[in] start the address of the first byte to sum
 *
 * \param[in] length the number of bytes to sum
 *
 * \return the sum
 */
uint32_t flash_sum(uint32_t start, uint32_t length);

#endif

