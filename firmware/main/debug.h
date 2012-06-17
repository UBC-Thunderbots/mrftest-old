#ifndef DEBUG_H
#define DEBUG_H

#include "io.h"
#include <stdint.h>

/**
 * \brief Sends a character over the debug port
 *
 * \param[in] ch the character to send
 */
static inline void debug_send(char ch) {
	outb(DEBUG_CTL, 0x01);
	outb(DEBUG_DATA, ch);
	while (inb(DEBUG_CTL) & 0x02);
	outb(DEBUG_CTL, 0x00);
}

#endif

