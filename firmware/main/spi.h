#ifndef SPI_H
#define SPI_H

/**
 * \file
 *
 * \brief Provides SPI communication capabilities.
 */

#include <stdint.h>

/**
 * \brief Initializes the SPI module.
 *
 * The bus is initially driven.
 */
void spi_init(void);

/**
 * \brief Tristates the SPI bus to allow another device to act as master.
 */
void spi_tristate(void);

/**
 * \brief Drives the SPI bus.
 */
void spi_drive(void);

/**
 * \brief Sends a byte over the SPI bus.
 *
 * \param[in] ch the byte to send.
 */
void spi_send(uint8_t ch) __wparam;

/**
 * \brief Receives a byte over the SPI bus.
 *
 * \return the received byte.
 */
uint8_t spi_receive(void);

#endif

