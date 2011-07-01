#ifndef PARBUS_H
#define PARBUS_H

#include <pic18fregs.h>
#include <stdint.h>

static inline void parbus_write(uint8_t reg, uint16_t value) {
	while (PMMODEHbits.BUSY);
	PMDIN1L = reg;
	while (PMMODEHbits.BUSY);
	PMDIN1L = value;
	while (PMMODEHbits.BUSY);
	PMDIN1L = value >> 8;
}

static inline uint16_t parbus_read(uint8_t reg) {
	uint8_t temp;

	while (PMMODEHbits.BUSY);
	PMDIN1L = reg;
	while (PMMODEHbits.BUSY);
	(void) PMDIN1L;
	while (PMMODEHbits.BUSY);
	temp = PMDIN1L;
	while (PMMODEHbits.BUSY);
	return (PMDIN1L << 8) | temp;
}

#endif

