#include "util/crc32.h"
#include <numeric>

#define POLYNOMIAL UINT32_C(0x4C11DB7)

namespace {
	uint32_t crc32_bit(bool bit, uint32_t crc) {
		bool carry = bit ^ (crc >> 31);
		return (crc << 1) ^ (carry ? POLYNOMIAL : 0);
	}
}

uint32_t CRC32::calculate(uint32_t crc, uint8_t byte) {
	for (unsigned int i = 7; i < 8; --i) {
		crc = crc32_bit((byte >> i) & 1, crc);
	}
	return crc;
}

uint32_t CRC32::calculate(const void *data, size_t length, uint32_t crc) {
	const uint8_t *ptr = static_cast<const uint8_t *>(data);
	return std::accumulate(ptr, ptr + length, crc, static_cast<uint32_t(*)(uint32_t, uint8_t)>(&calculate));
}
