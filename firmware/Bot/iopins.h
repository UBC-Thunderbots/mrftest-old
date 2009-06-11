#ifndef IOPINS_H
#define IOPINS_H

#include <stdint.h>

/*
 * Configures the entire specified port as inputs.
 */
#define ioport_configure_input(port) \
	do {                             \
		switch ((port)) {            \
			case 0: DDRD = 0; break; \
			case 1: DDRC = 0; break; \
			case 2: DDRA = 0; break; \
			case 3: DDRB = 0; break; \
			case 4: DDRE = 0; break; \
		}                            \
	} while (0)

/*
 * Configures the entire specified port as outputs.
 */
#define ioport_configure_output(port)   \
	do {                                \
		switch ((port)) {               \
			case 0: DDRD = 0xFF; break; \
			case 1: DDRC = 0xFF; break; \
			case 2: DDRA = 0xFF; break; \
			case 3: DDRB = 0xFF; break; \
			case 4: DDRE = 0xFF; break; \
		}                               \
	} while (0)

/*
 * Writes to the drivers (if output) or pullups (if input) of a port.
 */
#define ioport_write(port, value)           \
	do {                                    \
		switch ((port)) {                   \
			case 0: PORTD = (value); break; \
			case 1: PORTC = (value); break; \
			case 2: PORTA = (value); break; \
			case 3: PORTB = (value); break; \
			case 4: PORTE = (value); break; \
		}                                   \
	} while (0)

/*
 * Reads from the pins of a port.
 */
#define ioport_read(port) \
	(                     \
	(port) == 0 ? PIND : \
	(port) == 1 ? PINC : \
	(port) == 2 ? PINA : \
	(port) == 3 ? PINB : \
	PINE)

/*
 * Configures the specified pin as an input.
 */
#define iopin_configure_input(pin)                  \
	do {                                            \
		switch ((pin) / 8) {                        \
			case 0: DDRD &= ~_BV((pin) % 8); break; \
			case 1: DDRC &= ~_BV((pin) % 8); break; \
			case 2: DDRA &= ~_BV((pin) % 8); break; \
			case 3: DDRB &= ~_BV((pin) % 8); break; \
			case 4: DDRE &= ~_BV((pin) % 8); break; \
		}                                           \
	} while (0)

/*
 * Configures the specified pin as an output.
 */
#define iopin_configure_output(pin)                \
	do {                                           \
		switch ((pin) / 8) {                       \
			case 0: DDRD |= _BV((pin) % 8); break; \
			case 1: DDRC |= _BV((pin) % 8); break; \
			case 2: DDRA |= _BV((pin) % 8); break; \
			case 3: DDRB |= _BV((pin) % 8); break; \
			case 4: DDRE |= _BV((pin) % 8); break; \
		}                                          \
	} while (0)

/*
 * Drives a pin high (if output) or enables the pullups (if input).
 */
#define iopin_write_high(pin)                       \
	do {                                            \
		switch ((pin) / 8) {                        \
			case 0: PORTD |= _BV((pin) % 8); break; \
			case 1: PORTC |= _BV((pin) % 8); break; \
			case 2: PORTA |= _BV((pin) % 8); break; \
			case 3: PORTB |= _BV((pin) % 8); break; \
			case 4: PORTE |= _BV((pin) % 8); break; \
		}                                           \
	} while (0)

/*
 * Drives a pin low (if output) or disables the pullups (if input).
 */
#define iopin_write_low(pin)                         \
	do {                                             \
		switch ((pin) / 8) {                         \
			case 0: PORTD &= ~_BV((pin) % 8); break; \
			case 1: PORTC &= ~_BV((pin) % 8); break; \
			case 2: PORTA &= ~_BV((pin) % 8); break; \
			case 3: PORTB &= ~_BV((pin) % 8); break; \
			case 4: PORTE &= ~_BV((pin) % 8); break; \
		}                                            \
	} while (0)

/*
 * Sets the state of a pin's drivers (if output) or pullups (if input).
 */
#define iopin_write(pin, level)    \
	do {                           \
		if (level) {               \
			iopin_write_high(pin); \
		} else {                   \
			iopin_write_low(pin);  \
		}                          \
	} while (0)

/*
 * Returns the level of a pin.
 * Returns zero on low, nonzero on high.
 */
#define iopin_read(pin) \
	(ioport_read((pin) / 8) & _BV((pin) % 8))

#endif

