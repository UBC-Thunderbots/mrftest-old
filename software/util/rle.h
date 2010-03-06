#ifndef UTIL_RLE_H
#define UTIL_RLE_H

#include <cstddef>
#include <vector>
#include <stdint.h>

//
// Run-length-encodes a block of binary data. The expected usage model is:
//
// 1. Compress the data by passing it to the rle_compressor constructor.
// 2. Repeatedly fetch a block of compressed data using the next() function.
// 3. After each call to next(), call done() to check whether all compressed
//    data has been fetched or not.
// 4. Destroy the object.
//
class rle_compressor {
	public:
		//
		// Constructs a new rle_compressor by compressing the provided block of
		// binary data. The compressor does not keep a copy of the data but may
		// hold pointers to subregions of it; thus, the data buffer must be kept
		// intact until the compressor is destroyed.
		//
		rle_compressor(const void *data, std::size_t length);

		//
		// Indicates whether all compressed data has already been fetched.
		//
		bool done() const;

		//
		// Fetches the next block of compressed data. The output can be
		// constrained to any size, but larger output block sizes will result in
		// more efficient compression and in order to guarantee that progress
		// will be made, the output block size must be at least three bytes.
		//
		std::size_t next(void *buffer, std::size_t length);

	private:
		class rle_run {
			public:
				rle_run(bool is_repeat, std::size_t length, const unsigned char *data, uint16_t crc = 0);
				bool done() const;
				std::size_t encode(void *buffer, std::size_t buflen);

			private:
				bool is_repeat;
				std::size_t length;
				const unsigned char *data;
				uint16_t crc;
		};

		std::vector<rle_run> runs;
		unsigned int cur_run;
};

#endif

