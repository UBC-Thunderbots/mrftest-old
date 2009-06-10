#include <avr/io.h>
#include <avr/interrupt.h>

#include "constants.h"
#include "debug.h"

static volatile char buffer[DEBUG_BUFSIZE];
static volatile uint8_t wptr, rptr;

ISR(USART1_UDRE_vect, ISR_BLOCK) {
	UDR1 = buffer[rptr];
	rptr = (rptr + 1) % DEBUG_BUFSIZE;
	if (rptr == wptr)
		UCSR1B &= ~_BV(UDRIE1);
}

void debug_init(void) {
	UBRR1H = ((F_CPU + 4 * DEBUG_BAUD) / (8 * DEBUG_BAUD) - 1) / 256;
	UBRR1L = ((F_CPU + 4 * DEBUG_BAUD) / (8 * DEBUG_BAUD) - 1) % 256;
	UCSR1A = _BV(U2X1);
	UCSR1B = _BV(TXEN1);
	UCSR1C = _BV(USBS1) | _BV(UCSZ11) | _BV(UCSZ10);
}

void debug_putc(char ch) {
	uint8_t new_wptr = (wptr + 1) % DEBUG_BUFSIZE;
	if (new_wptr != rptr) {
		buffer[wptr] = ch;
		wptr = new_wptr;
		UCSR1B |= _BV(UDRIE1);
	}
}

void debug_puts(const char *str) {
	while (*str)
		debug_putc(*str++);
}

void debug_puti(int32_t i) {
	char str[11];
	char *p;

	str[10] = '\0';
	p = &str[9];
	do {
		*p-- = (i % 10) + '0';
		i /= 10;
	} while (i);
	debug_puts(p + 1);
}

void debug_putf(double f) {
	uint8_t i;

	debug_puti((int) f);
	f -= (int) f;
	if (f < 0)
		f = -f;

	i = 6;
	do {
		f *= 10;
		debug_putc((int) f + '0');
	} while (--i);
}

