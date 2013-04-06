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

#endif

