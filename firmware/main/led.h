#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	HALL_SENSORS,
	ENCODERS,
	USER_MODE
} led_mode_t;

static inline void radio_led_ctl(bool state) {
	uint8_t prev_value = inb(LED_CTL);
	outb(LED_CTL, (state)?(0x80|prev_value):(0x7F&prev_value));
}

static inline void radio_led_blink() {
	outb(LED_CTL,0x40|inb(LED_CTL));
}

static inline void set_test_leds(led_mode_t led_mode, uint8_t mode) {
	uint8_t value = 0x00;
	switch(led_mode) {
		case USER_MODE:
			value |= 0x20;
		case HALL_SENSORS:
			outb(LED_CTL, value|mode);
			break;
		case ENCODERS:
			outb(LED_CTL, value|(mode+5));
			break;
	}
}

#endif
