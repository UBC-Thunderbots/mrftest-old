#ifndef PARBUS_H
#define PARBUS_H

#include <pic18fregs.h>
#include <stdint.h>

static inline void parbus_write(uint8_t reg, uint16_t value) {
	PMDIN1L = reg;
	PMDIN1L = value;
	PMDIN1L = value >> 8;
}

static inline uint16_t parbus_read(uint8_t reg) {
	uint8_t temp;

	PMDIN1L = reg;
	Nop();
	(void) PMDIN1L;
	Nop();
	temp = PMDIN1L;
	Nop();
	return (PMDIN1L << 8) | temp;
}

#endif

