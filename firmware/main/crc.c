#include "crc.h"

uint16_t crc_update(uint16_t crc, uint8_t ch) {
	ch ^= crc;
	ch ^= ch << 4;
	crc = crc >> 8;
	crc |= ch << 8;
	crc ^= ch << 3;
	crc ^= ch >> 4;
	return crc;
}

