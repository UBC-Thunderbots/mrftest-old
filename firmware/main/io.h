#ifndef IO_H
#define IO_H

/**
 * \file
 *
 * \brief Provides access to hardware I/O ports
 */

#include <stdint.h>

/**
 * \brief Reads from an I/O port
 *
 * \param[in] port the port to read from
 *
 * \return the port’s current value
 */
static uint8_t inb(uint8_t port) __attribute__((unused));

/**
 * \brief Writes to an I/O port
 *
 * \param[in] port the port to write to
 *
 * \param[in] value the value to write
 */
static void outb(uint8_t port, uint8_t value) __attribute__((unused));



static inline uint8_t inb(uint8_t port) {
	uint8_t value;
	asm("in %0, %1" : "=r"(value) : "i"(port));
	return value;
}

static inline void outb(uint8_t port, uint8_t value) {
	asm volatile("out %0, %1" : : "i"(port), "r"(value));
}

#endif

