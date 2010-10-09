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

uint16_t CRC16::calculate(const void *buf, std::size_t len) {
	return calculate(buf, len, INITIAL);
}

uint16_t CRC16::calculate(const void *buf, std::size_t len, uint16_t crc) {
	const uint8_t *buffer = static_cast<const uint8_t *>(buf);
	while (len--) {
		crc = crc16_byte(crc, *buffer++);
	}
	return crc;
}

