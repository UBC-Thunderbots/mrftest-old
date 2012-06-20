#ifndef CHICKER_H
#define CHICKER_H

#include <stdbool.h>
#include <stdint.h>

#define PRESERVE_MASK 0x21

static inline uint8_t get_preserved_bits() {
	return inb(CHICKER_CTL) & PRESERVE_MASK;
}

static inline void set_charge_mode(bool enable) {
	outb(CHICKER_CTL, (get_preserved_bits() & ~0x01) | (enable? 0x01: 0x00));
}

static inline void set_discharge_mode(bool enable) {
	outb(CHICKER_CTL, (get_preserved_bits() & ~0x20) | (enable? 0x20: 0x00));
}

static inline bool is_charged() {
	return ( inb(CHICKER_CTL) & 0x10 );
}

static inline bool is_charge_timeout() {
	return ( inb(CHICKER_CTL) & 0x08 );                                          
}

static inline void set_chick_pulse(uint16_t pulsewidth) {
	outb( CHICKER_PULSE_LSB, pulsewidth & 0xFF );
	outb( CHICKER_PULSE_MSB, pulsewidth >> 8);
}

static inline void fire_kicker() {
	outb(CHICKER_CTL, get_preserved_bits() | 0x02);	
}

static inline void fire_chipper() {
	outb(CHICKER_CTL, get_preserved_bits() | 0x04);
}

#endif

