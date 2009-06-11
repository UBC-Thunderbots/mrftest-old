#include <avr/io.h>

#include "constants.h"
#include "pwmpins.h"
#include "iopins.h"

void pwm_init(void) {
	// 10-bit phase-correct PWM mode, TOP fixed at 0x3FF, non-inverted output, clock to FOSC/8.
	TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1C1) | _BV(WGM11) | _BV(WGM10);
	TCCR1B = _BV(CS11);
	TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1) | _BV(WGM31) | _BV(WGM30);
	TCCR3B = _BV(CS31);

	// 8-bit phase-correct PWM mode, TOP fixed at 0xFF, non-inverted output, clock to FOSC/8.
	TCCR0 = _BV(WGM00) | _BV(COM01) | _BV(CS01);

	// Outputs.
	iopin_configure_output(IOPIN_PWM0);
	iopin_configure_output(IOPIN_PWM1);
	iopin_configure_output(IOPIN_PWM2);
	iopin_configure_output(IOPIN_PWM3);
	iopin_configure_output(IOPIN_PWM4);
	iopin_configure_output(IOPIN_PWM5);
	iopin_configure_output(IOPIN_PWM6);
}

