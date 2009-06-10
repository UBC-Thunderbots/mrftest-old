#ifndef LED_H
#define LED_H

#define led_init()      \
	do {                \
		DDRG |= _BV(0); \
	} while (0)

#define led_on()         \
	do {                 \
		PORTG |= _BV(0); \
	} while (0)

#define led_off()         \
	do {                  \
		PORTG &= ~_BV(0); \
	} while (0)

#endif

