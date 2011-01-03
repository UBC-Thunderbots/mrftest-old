#include "activity_leds.h"
#include "pins.h"
#include "signal.h"
#include <pic18fregs.h>

/**
 * \brief The bitmask of XBees on which activity has recently taken place and whether the LEDs are on or off.
 */
static volatile uint8_t flags = 0;

void activity_leds_init(void) {
	/* Configure timer 0 to run at a period of 65536 × 8 / 12000000 =~ 44ms.
	 *
	 *        /-------- Timer on
	 *        |/------- 16-bit
	 *        ||/------ Internal instruction clock
	 *        |||/----- Ignored
	 *        ||||/---- Prescaler assigned
	 *        |||||///- Prescale 1:8 */
	T0CON = 0b10000010;

	/* Enable interrupts and make them low priority. */
	INTCON2bits.TMR0IP = 0;
	INTCONbits.TMR0IE = 1;
}

void activity_leds_deinit(void) {
	/* Turn off the timer and interrupts. */
	T0CONbits.TMR0ON = 0;
	INTCONbits.TMR0IE = 0;
}

void activity_leds_mark(uint8_t xbee) {
	/* Record activity on the given XBee. */
	if (xbee == 0) {
		flags |= 0x1;
	} else /*if (xbee == 1)*/ {
		flags |= 0x2;
	}
}

SIGHANDLER(activity_leds_tmr0if) {
	if (flags & 0x4) {
		LAT_LED2 = 1;
		LAT_LED3 = 1;
		flags &= ~0x4;
	} else {
		if (flags & 0x1) {
			LAT_LED2 = 0;
			flags &= ~0x1;
		}
		if (flags & 0x2) {
			LAT_LED3 = 0;
			flags &= ~0x2;
		}
		flags |= 0x4;
	}
	INTCONbits.TMR0IF = 0;
}

