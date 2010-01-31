#ifndef UTIL_CODEC_H
#define UTIL_CODEC_H

#include <vector>
#include <cstddef>
#include <stdint.h>

namespace {
	void encode_u8(std::vector<uint8_t> &v, uint8_t x) {
		v.push_back(x);
	}

	void encode_u16(std::vector<uint8_t> &v, uint16_t x) {
		encode_u8(v, x >> 8);
		encode_u8(v, x);
	}

	void encode_u32(std::vector<uint8_t> &v, uint32_t x) {
		encode_u16(v, x >> 16);
		encode_u16(v, x);
	}

	void encode_u64(std::vector<uint8_t> &v, uint64_t x) {
		encode_u32(v, x >> 32);
		encode_u32(v, x);
	}

	uint8_t decode_u8(const uint8_t *buffer, std::size_t &i) {
		return buffer[i++];
	}

	uint16_t decode_u16(const uint8_t *buffer, std::size_t &i) {
		uint16_t high = decode_u8(buffer, i);
		uint16_t low = decode_u8(buffer, i);
		return (high << 8) | low;
	}

	uint32_t decode_u32(const uint8_t *buffer, std::size_t &i) {
		uint32_t high = decode_u16(buffer, i);
		uint32_t low = decode_u16(buffer, i);
		return (high << 16) | low;
	}

	uint64_t decode_u64(const uint8_t *buffer, std::size_t &i) {
		uint64_t high = decode_u32(buffer, i);
		uint64_t low = decode_u32(buffer, i);
		return (high << 32) | low;
	}
}

#endif

