#include <avr/io.h>

#include "pwmpins.h"

void pwm_init(void) {
	// 10-bit phase-correct PWM mode, TOP fixed at 0x3FF, non-inverted output, clock to FOSC/8.
	TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1C1) | _BV(WGM11) | _BV(WGM10);
	TCCR1B = _BV(CS11);
	TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1) | _BV(WGM31) | _BV(WGM30);
	TCCR3B = _BV(CS31);

	// Outputs.
	DDRB |= _BV(7) | _BV(6) | _BV(5);
	DDRE |= _BV(5) | _BV(4) | _BV(3);
}

