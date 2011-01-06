#ifndef LEDS_H
#define LEDS_H

/**
 * \file
 *
 * \brief Provides the ability to display feedback on the LEDs.
 */

/**
 * \brief Shows a number between 0 and 15 on the LEDs.
 *
 * \param[in] n the number to display.
 */
void leds_show_number(unsigned char n);

#endif

