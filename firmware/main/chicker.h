#ifndef CHICKER_H
#define CHICKER_H

#include <stdbool.h>
#include <stdint.h>

#define PRESERVE_MASK 0x21

static inline uint8_t get_preserved_bits() {
	return CHICKER_CTL & PRESERVE_MASK;
}

static inline void set_charge_mode(bool enable) {
	CHICKER_CTL = (get_preserved_bits() & ~0x01) | (enable ? 0x01 : 0x00);
}

static inline void set_discharge_mode(bool enable) {
	CHICKER_CTL = (get_preserved_bits() & ~0x20) | (enable ? 0x20 : 0x00);
}

static inline bool is_charged() {
	return !!(CHICKER_CTL & 0x10);
}

static inline bool is_charge_timeout() {
	return !!(CHICKER_CTL & 0x08);
}

static inline void set_chick_pulse(uint16_t pulsewidth) {
	CHICKER_PULSE_LSB = pulsewidth & 0xFF;
	CHICKER_PULSE_MSB = pulsewidth >> 8;
}

static inline void fire_kicker() {
	CHICKER_CTL = get_preserved_bits() | 0x02;
}

static inline void fire_chipper() {
	CHICKER_CTL = get_preserved_bits() | 0x04;
}

#endif

