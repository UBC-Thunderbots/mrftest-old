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
 * \brief Whether the USB layer has entered the main configuration and the main loop should start configuring XBees and processing packets.
 */
extern volatile BOOL should_start_up;

/**
 * \brief Whether the USB layer has exited the main configuration and the main loop should stop processing packets and shut down the XBees.
 */
extern volatile BOOL should_shut_down;

/**
 * \brief The channels the host asked the XBees to be configured on.
 */
extern volatile uint8_t requested_channels[2];

/**
 * \brief The firmware versions of the XBees.
 */
extern uint16_t xbee_versions[2];

/**
 * \brief Checks if the USB subsystem has become idle and, if so, puts the dongle to sleep until the USB host comes back.
 */
void check_idle(void);

#endif

