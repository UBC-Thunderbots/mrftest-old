#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#include "constants.h"
#include "test.h"
#include "debug.h"
#include "iopins.h"

static double data[512];
static uint16_t ptr;

#define ee_ready *((uint8_t *) (0))
#define ee_data ((double *) (2))
#define READY_VALUE 0x57

uint8_t test_has_run(void) {
	return eeprom_read_byte(&ee_ready) == READY_VALUE;
}

void test_add(double elem) {
	if (ptr < sizeof(data) / sizeof(*data)) {
		data[ptr++] = elem;
	}
}

uint8_t test_full(void) {
	return ptr == sizeof(data) / sizeof(*data);
}

void test_save(void) {
	// Turn on the LED.
	iopin_write(IOPIN_LED, 1);

	// Save the data into EEPROM.
	eeprom_write_block(data, ee_data, sizeof(data));

	// Mark data as pending.
	eeprom_write_byte(&ee_ready, READY_VALUE);

	// Turn off the LED.
	iopin_write(IOPIN_LED, 0);

	// Die.
	for (;;)
		sleep_mode();
}

void test_dump(void) {
	uint8_t i;
	double elem;

	// Enable serial port receiver.
	UCSR0B |= _BV(RXEN0);

	// Wait until three bytes are received.
	for (i = 0; i < 3; i++) {
		while (!(UCSR0A & _BV(RXC0)));
		UDR0;
	}

	// Dump the data.
	for (ptr = 0; ptr < sizeof(data) / sizeof(*data); ptr++) {
		eeprom_read_block(&elem, &ee_data[ptr], sizeof(double));
		debug_putf(elem);
		debug_putc('\n');
		debug_flush();
	}

	// Clear the data ready flag
	eeprom_write_byte(&ee_ready, 0);

	// Shut down the CPU.
	for (;;) {
		sleep_mode();
	}
}

