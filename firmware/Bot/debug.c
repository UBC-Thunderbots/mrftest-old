#include <avr/io.h>
#include <avr/interrupt.h>

#include "constants.h"
#include "debug.h"

static volatile char buffer[DEBUG_BUFSIZE];
static volatile uint8_t wptr, rptr;

ISR(USART0_UDRE_vect, ISR_BLOCK) {
	UDR0 = buffer[rptr];
	rptr = (rptr + 1) % DEBUG_BUFSIZE;
	if (rptr == wptr)
		UCSR0B &= ~_BV(UDRIE0);
}

void debug_init(void) {
#define DEBUG_BAUD_DIVISOR (((F_CPU + 4UL * DEBUG_BAUD) / (8UL * DEBUG_BAUD)) - 1UL)
	UBRR0H = DEBUG_BAUD_DIVISOR / 256UL;
	UBRR0L = DEBUG_BAUD_DIVISOR % 256UL;
	UCSR0A = _BV(U2X0);
	UCSR0B = _BV(TXEN0);
	UCSR0C = _BV(USBS0) | _BV(UCSZ01) | _BV(UCSZ00);
}

void debug_putc(char ch) {
	uint8_t new_wptr = (wptr + 1) % DEBUG_BUFSIZE;
	if (new_wptr != rptr) {
		buffer[wptr] = ch;
		wptr = new_wptr;
		UCSR0B |= _BV(UDRIE0);
	}
}

void debug_puts(const char *str) {
	while (*str)
		debug_putc(*str++);
}

void debug_puti(int32_t i) {
	char str[11];
	char *p;

	if (i < 0) {
		debug_putc('-');
		i = -i;
	}

	str[10] = '\0';
	p = &str[9];
	do {
		*p-- = (i % 10) + '0';
		i /= 10;
	} while (i);
	debug_puts(p + 1);
}

void debug_puth(uint32_t i) {
	static const char HEX_DIGITS[16] = "0123456789ABCDEF";
	char str[9];
	char *p;

	str[8] = '\0';
	p = &str[8];
	do {
		*--p = HEX_DIGITS[i % 16];
		i /= 16;
	} while (p != str);
	debug_puts(str);
}

void debug_putf(double f) {
	uint8_t i;
	int32_t elem;

	elem = f;
	debug_puti(elem);
	f -= elem;
	if (f < 0)
		f = -f;

	debug_putc('.');

	i = 6;
	do {
		f *= 10;
		elem = f;
		debug_putc(elem + '0');
		f -= elem;
	} while (--i);
}

void debug_flush(void) {
	while (wptr != rptr);
}

