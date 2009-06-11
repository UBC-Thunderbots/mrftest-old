#include <avr/io.h>
#include <avr/interrupt.h>

#include "rtc.h"

volatile unsigned long rtc_counter;

ISR(TIMER2_COMP_vect, ISR_BLOCK) {
	rtc_counter++;
}

void rtc_init(void) {
#define PRESCALE_BITS (_BV(CS21) | _BV(CS20));
#define PRESCALE_VALUE 64
#if F_CPU % PRESCALE_VALUE != 0 || (F_CPU / PRESCALE_VALUE) % 1000 != 0 || F_CPU / PRESCALE_VALUE / 1000 > 255
#error Fix the math in this code!
#endif
	// NOTE: We're using the primary 16MHz crystal here.
	// Should we use the 32768Hz crystal on TOsc?
	// It's probably a precision crystal, but it would give worse theoretical results (not exactly 1ms)!
	TCCR2 = _BV(WGM21) | PRESCALE_BITS;
	OCR2 = F_CPU / PRESCALE_VALUE / 1000;
	TIMSK |= _BV(OCIE2);
}

