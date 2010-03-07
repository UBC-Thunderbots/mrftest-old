#include "util/crc16.h"
#include "util/rle.h"
#include <algorithm>
#include <cassert>
#include <utility>



rle_compressor::rle_compressor(const void *data, std::size_t length) : cur_run(0) {
	// Compute the CRC16 of the input data.
	uint16_t crc = crc16::calculate(data, length);

	// First, break the input into a sequence of blocks where all bytes in the
	// same block are equal.
	std::vector<std::pair<std::size_t, const unsigned char *> > blocks;
	{
		const unsigned char *src = static_cast<const unsigned char *>(data);
		while (length--) {
			if (!blocks.empty() && *src == *blocks.back().second) {
				// This byte matches the current block.
				++blocks.back().first;
			} else {
				// No current block, or mismatch. Start a new block.
				blocks.push_back(std::make_pair(1U, src));
			}
			++src;
		}
	}

	// Now turn the blocks into runs by electing, based on the length of each
	// block, whether to generate a repeat run or whether to generate or add to
	// a literal run.
	{
		unsigned int block_num = 0;
		while (block_num < blocks.size()) {
			// It's worth generating a repeat run if the block is 3 bytes long.
			if (blocks[block_num].first >= 3) {
				runs.push_back(rle_run(true, blocks[block_num].first, blocks[block_num].second));
				++block_num;
			} else {
				// Want to generate a literal run; make it as long as we can.
				const unsigned char *src = blocks[block_num].second;
				std::size_t length = 0;
				while (block_num < blocks.size() && blocks[block_num].first < 3) {
					length += blocks[block_num].first;
					++block_num;
				}
				runs.push_back(rle_run(false, length, src));
			}
		}
	}

	// Add the termination marker.
	runs.push_back(rle_run(false, 1, 0, crc));
}

bool rle_compressor::done() const {
	// We're finished when we have no more runs to encode.
	return cur_run == runs.size();
}

std::size_t rle_compressor::next(void *buffer, std::size_t length) {
	// Sanity check.
	assert(!done());

	// Push through the runs.
	std::size_t encoded = 0;
	unsigned char *bufptr = static_cast<unsigned char *>(buffer);
	for (;;) {
		// Try to encode the current run into the buffer.
		encoded += runs[cur_run].encode(bufptr + encoded, length - encoded);

		if (runs[cur_run].done()) {
			// The current run has finished encoding itself. Advance.
			++cur_run;
			if (done()) {
				// All runs are now finished. No more work can be done.
				return encoded;
			} else {
				// More runs to go. Keep working.
			}
		} else {
			// The current run has more encoding to do. If it were able to do
			// it, it would have already done it. That it returned without
			// finishing itself signals that we have run out of buffer space.
			// Thus, return immediately.
			return encoded;
		}
	}
}



rle_compressor::rle_run::rle_run(bool is_repeat, std::size_t length, const unsigned char *data, uint16_t crc) : is_repeat(is_repeat), length(length), data(data), crc(crc) {
}

bool rle_compressor::rle_run::done() const {
	// We're finished when all data for this run has been encoded.
	return !length;
}

std::size_t rle_compressor::rle_run::encode(void *buffer, std::size_t buflen) {
	unsigned char *bufptr = static_cast<unsigned char *>(buffer);
	assert(!done());

	if (is_repeat) {
		// Here, "length" is the number of occurrences of the repeating byte
		// that have yet to be encoded. Keep pushing until we run out of either
		// occurrences or buffer space.
		std::size_t generated = 0;
		while (buflen >= 2 && length > 0) {
			// We can only encode up to 127 occurrences in one output chunk.
			unsigned char s = std::min<std::size_t>(127U, length);
			*bufptr++ = s | 0x80;
			*bufptr++ = *data;
			buflen -= 2;
			length -= s;
			generated += s;
		}
		return generated;
	} else if (data) {
		// Here, "length" is the number of bytes that have yet to be encoded.
		// Keep pushing until we run out of either data or buffer space.
		std::size_t generated = 0;
		while (buflen >= 2 && length > 0) {
			// We can only encode up to 127 bytes in one output chunk. We are
			// also not allowed to overflow the output buffer.
			unsigned char s = std::min<std::size_t>(127U, std::min<std::size_t>(buflen - 1, length));
			*bufptr++ = s;
			std::copy(data, data + s, bufptr);
			bufptr += s;
			buflen -= s + 1;
			length -= s;
			generated += s + 1;
		}
		return generated;
	} else {
		// This is a termination marker. Here, "length" is set to 1 if the
		// marker has not yet been written, or 0 if it has. Writing the marker
		// requires 3 bytes of buffer space (one for the code and two for the
		// CRC).
		if (buflen >= 3) {
			bufptr[0] = 0;
			bufptr[1] = crc / 256;
			bufptr[2] = crc % 256;
			length = 0;
			return 3;
		} else {
			return 0;
		}
	}
}

