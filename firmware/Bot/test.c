#include <avr/io.h>
#include <avr/sleep.h>

#include "test.h"
#include "debug.h"

static double data[512];
static uint16_t ptr;

void test_add(double elem) {
	if (ptr < sizeof(data) / sizeof(*data)) {
		data[ptr++] = elem;
	}
}

uint8_t test_full(void) {
	return ptr == sizeof(data) / sizeof(*data);
}

void test_dump(void) {
	uint8_t i;

	// Enable serial port receiver.
	UCSR0B |= _BV(RXEN0);

	// Wait until three bytes are received.
	for (i = 0; i < 3; i++) {
		while (!(UCSR0A & _BV(RXC0)));
		UDR0;
	}

	// Dump the data.
	for (ptr = 0; ptr < sizeof(data) / sizeof(*data); ptr++) {
		debug_putf(data[ptr]);
		debug_putc('\n');
		debug_flush();
	}

	// Shut down the CPU.
	for (;;) {
		sleep_mode();
	}
}

