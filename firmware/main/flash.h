#ifndef SPI_H
#define SPI_H

/**
 * \file
 *
 * \brief Provides the ability to send and receive data over SPI
 */

#include "io.h"
#include <stdint.h>

/**
 * \brief Asserts chip select to the SPI flash
 */
static inline void flash_assert_cs(void) {
	outb(FLASH_CTL, 0x00);
}

/**
 * \brief Deasserts chip select to the SPI flash
 */
static inline void flash_deassert_cs(void) {
	outb(FLASH_CTL, 0x02);
}

/**
 * \brief Sends one byte to the SPI flash
 *
 * \param[in] b the byte to send
 */
static inline void flash_tx(uint8_t b) {
	outb(FLASH_DATA, b);
	while (inb(FLASH_CTL) & 0x01);
}

/**
 * \brief Sends and receives one byte to the SPI flash
 *
 * \param[in] b the byte to send
 *
 * \return the received byte
 */
static inline uint8_t flash_txrx(uint8_t b) {
	flash_tx(b);
	return inb(FLASH_DATA);
}

#endif

