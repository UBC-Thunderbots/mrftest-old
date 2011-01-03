#ifndef DEBUG_H
#define DEBUG_H

/**
 * \file
 *
 * \brief Provides debugging output capabilities.
 */

#include <stdbool.h>
#include <stdio.h>

/**
 * \brief Whether or not debug output is currently enabled.
 */
extern volatile BOOL debug_enabled;

/**
 * \brief Initializes the subsystem.
 */
void debug_init(void);

/**
 * \brief Enables debug output.
 */
void debug_enable(void);

/**
 * \brief Disables debug output.
 */
void debug_disable(void);

/**
 * \brief Prints a debugging message.
 */
#define DPRINTF(...) do { if (debug_enabled) { printf_tiny(__VA_ARGS__); } } while (0)

#endif

