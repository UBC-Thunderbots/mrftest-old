#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

/*
 * Initializes the debug module.
 */
void debug_init(void);

/*
 * Outputs one character.
 */
void debug_putc(char ch);

/*
 * Outputs a string.
 */
void debug_puts(const char *str);

/*
 * Outputs an integer.
 */
void debug_puti(int32_t i);

/*
 * Outputs an integer in hex.
 */
void debug_puth(uint32_t i);

/*
 * Outputs a floating-point number.
 */
void debug_putf(double f);

/*
 * Waits until the output buffer is empty.
 */
void debug_flush(void);

#endif

