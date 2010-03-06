#include "util/crc16.h"

namespace {
	uint16_t crc16_byte(uint16_t crc, uint8_t data) {
		data ^= crc;
		data ^= data << 4;
		crc >>= 8;
		crc |= data << 8;
		crc ^= data << 3;
		crc ^= data >> 4;
		return crc;
	}
}

uint16_t crc16::calculate(const void *buf, std::size_t len) {
	const uint8_t *buffer = static_cast<const uint8_t *>(buf);
	uint16_t crc = 0xFFFF;
	while (len--)
		crc = crc16_byte(crc, *buffer++);
	return crc;
}

