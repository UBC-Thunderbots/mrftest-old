#ifndef GLOBAL_H
#define GLOBAL_H

/**
 * \file
 *
 * \brief Various global communication channels between modules and globally useful functions.
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief Whether the main loop should be running or stopped.
 */
extern volatile BOOL should_run;

/**
 * \brief The firmware versions of the XBees.
 */
extern uint16_t xbee_versions[2];

#endif

