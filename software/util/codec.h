#ifndef UTIL_CODEC_H
#define UTIL_CODEC_H

#include <cstddef>
#include <stdint.h>
#include <vector>

namespace {
	/**
	 * Encodes an 8-bit integer to a byte array.
	 *
	 * \param[in,out] b the buffer into which to encode.
	 *
	 * \param[in] x the integer to encode.
	 */
	void encode_u8(uint8_t *b, uint8_t x) {
		b[0] = x;
	}

	/**
	 * Encodes a 16-bit integer to a byte array.
	 *
	 * \param[in,out] b the buffer into which to encode.
	 *
	 * \param[in] x the integer to encode.
	 */
	void encode_u16(uint8_t *b, uint16_t x) {
		encode_u8(b, x >> 8);
		encode_u8(b + 1, x);
	}

	/**
	 * Encodes a 32-bit integer to a byte array.
	 *
	 * \param[in,out] b the buffer into which to encode.
	 *
	 * \param[in] x the integer to encode.
	 */
	void encode_u32(uint8_t *b, uint32_t x) {
		encode_u16(b, x >> 16);
		encode_u16(b + 2, x);
	}

	/**
	 * Encodes a 64-bit integer to a byte array.
	 *
	 * \param[in,out] b the buffer into which to encode.
	 *
	 * \param[in] x the integer to encode.
	 */
	void encode_u64(uint8_t *b, uint64_t x) {
		encode_u32(b, x >> 32);
		encode_u32(b + 4, x);
	}

	/**
	 * Appends an 8-bit integer to a \c vector.
	 *
	 * \param[in,out] v the \c vector to append to.
	 *
	 * \param[in] x the integer to append.
	 */
	void encode_u8(std::vector<uint8_t> &v, uint8_t x) {
		v.push_back(x);
	}

	/**
	 * Appends a 16-bit integer to a \c vector.
	 *
	 * \param[in,out] v the \c vector to append to.
	 *
	 * \param[in] x the integer to append.
	 */
	void encode_u16(std::vector<uint8_t> &v, uint16_t x) {
		encode_u8(v, x >> 8);
		encode_u8(v, x);
	}

	/**
	 * Appends a 32-bit integer to a \c vector.
	 *
	 * \param[in,out] v the \c vector to append to.
	 *
	 * \param[in] x the integer to append.
	 */
	void encode_u32(std::vector<uint8_t> &v, uint32_t x) {
		encode_u16(v, x >> 16);
		encode_u16(v, x);
	}

	/**
	 * Appends a 64-bit integer to a \c vector.
	 *
	 * \param[in,out] v the \c vector to append to.
	 *
	 * \param[in] x the integer to append.
	 */
	void encode_u64(std::vector<uint8_t> &v, uint64_t x) {
		encode_u32(v, x >> 32);
		encode_u32(v, x);
	}

	/**
	 * Extracts an 8-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \return the integer.
	 */
	uint8_t decode_u8(const uint8_t *buffer) {
		return *buffer;
	}

	/**
	 * Extracts a 16-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \return the integer.
	 */
	uint16_t decode_u16(const uint8_t *buffer) {
		uint16_t high = decode_u8(buffer);
		uint16_t low = decode_u8(buffer + 1);
		return (high << 8) | low;
	}

	/**
	 * Extracts a 32-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \return the integer.
	 */
	uint32_t decode_u32(const uint8_t *buffer) {
		uint32_t high = decode_u16(buffer);
		uint32_t low = decode_u16(buffer + 2);
		return (high << 16) | low;
	}

	/**
	 * Extracts a 64-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \return the integer.
	 */
	uint64_t decode_u64(const uint8_t *buffer) {
		uint64_t high = decode_u32(buffer);
		uint64_t low = decode_u32(buffer + 4);
		return (high << 32) | low;
	}

	/**
	 * Extracts an 8-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \param[in,out] i an index into the buffer at which to extract, which will be incremented by the number of bytes consumed.
	 *
	 * \return the integer.
	 */
	uint8_t decode_u8(const uint8_t *buffer, std::size_t &i) {
		return buffer[i++];
	}

	/**
	 * Extracts a 16-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \param[in,out] i an index into the buffer at which to extract, which will be incremented by the number of bytes consumed.
	 *
	 * \return the integer.
	 */
	uint16_t decode_u16(const uint8_t *buffer, std::size_t &i) {
		uint16_t high = decode_u8(buffer, i);
		uint16_t low = decode_u8(buffer, i);
		return (high << 8) | low;
	}

	/**
	 * Extracts a 32-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \param[in,out] i an index into the buffer at which to extract, which will be incremented by the number of bytes consumed.
	 *
	 * \return the integer.
	 */
	uint32_t decode_u32(const uint8_t *buffer, std::size_t &i) {
		uint32_t high = decode_u16(buffer, i);
		uint32_t low = decode_u16(buffer, i);
		return (high << 16) | low;
	}

	/**
	 * Extracts a 64-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \param[in,out] i an index into the buffer at which to extract, which will be incremented by the number of bytes consumed.
	 *
	 * \return the integer.
	 */
	uint64_t decode_u64(const uint8_t *buffer, std::size_t &i) {
		uint64_t high = decode_u32(buffer, i);
		uint64_t low = decode_u32(buffer, i);
		return (high << 32) | low;
	}

	/**
	 * Extracts an 8-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \param[in,out] i an index into the buffer at which to extract, which will be incremented by the number of bytes consumed.
	 *
	 * \return the integer.
	 */
	uint8_t decode_u8(const std::vector<uint8_t> &buffer, std::size_t &i) {
		return decode_u8(&buffer[0], i);
	}

	/**
	 * Extracts a 16-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \param[in,out] i an index into the buffer at which to extract, which will be incremented by the number of bytes consumed.
	 *
	 * \return the integer.
	 */
	uint16_t decode_u16(const std::vector<uint8_t> &buffer, std::size_t &i) {
		return decode_u16(&buffer[0], i);
	}

	/**
	 * Extracts a 32-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \param[in,out] i an index into the buffer at which to extract, which will be incremented by the number of bytes consumed.
	 *
	 * \return the integer.
	 */
	uint32_t decode_u32(const std::vector<uint8_t> &buffer, std::size_t &i) {
		return decode_u32(&buffer[0], i);
	}

	/**
	 * Extracts a 64-bit integer from a data buffer.
	 *
	 * \param[in] buffer the data to extract from.
	 *
	 * \param[in,out] i an index into the buffer at which to extract, which will be incremented by the number of bytes consumed.
	 *
	 * \return the integer.
	 */
	uint64_t decode_u64(const std::vector<uint8_t> &buffer, std::size_t &i) {
		return decode_u64(&buffer[0], i);
	}
}

#endif

