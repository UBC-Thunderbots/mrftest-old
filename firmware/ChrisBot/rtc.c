#include <avr/io.h>
#include <avr/interrupt.h>

#include "rtc.h"

volatile unsigned long millis;

ISR(TIMER0_COMP_vect, ISR_BLOCK) {
	millis++;
}

void rtc_init(void) {
#if F_CPU % 128 != 0 || (F_CPU / 128) % 1000 != 0 || F_CPU / 128 / 1000 > 255
#error Fix the math in this code!
#endif
	// NOTE: We're using the primary 16MHz crystal here.
	// Should we use the 32768Hz crystal on TOsc?
	// It's probably a precision crystal, but it would give worse theoretical results (not exactly 1ms)!
	TCCR0 = _BV(WGM01) | _BV(CS02) | _BV(CS00);
	OCR0 = F_CPU / 128 / 1000;
	TIMSK |= _BV(OCIE0);
}

