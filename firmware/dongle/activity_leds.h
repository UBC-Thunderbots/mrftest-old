#ifndef ACTIVITY_LEDS_H
#define ACTIVITY_LEDS_H

/**
 * \file
 *
 * \brief Implements activity display by blinking the LEDs.
 */

#include <stdint.h>

/**
 * \brief Initializes activity display.
 */
void activity_leds_init(void);

/**
 * \brief Deinitializes activity display.
 */
void activity_leds_deinit(void);

/**
 * \brief Registers activity for an XBee.
 *
 * \param[in] xbee the index of the XBee on which activity occurred.
 */
void activity_leds_mark(uint8_t xbee);

#endif

