#ifndef PWMPINS_H
#define PWMPINS_H

#include <avr/interrupt.h>

/*
 * Initializes the PWM subsystem.
 */
void pwm_init(void);

/*
 * Sets the level on a PWM output pin.
 */
#define pwm_write(pin, level)               \
	do {                                    \
		cli();                              \
		switch ((pin)) {                    \
			case 0: OCR3C = (level); break; \
			case 1: OCR3B = (level); break; \
			case 2: OCR3A = (level); break; \
			case 3: OCR1C = (level); break; \
			case 4: OCR1B = (level); break; \
			case 5: OCR1A = (level); break; \
		}                                   \
		sei();                              \
	} while (0)

#endif

