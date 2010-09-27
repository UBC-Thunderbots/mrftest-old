#ifndef UTIL_RLE_H
#define UTIL_RLE_H

#include <cstddef>
#include <stdint.h>
#include <vector>

/**
 * Run-length-encodes a block of binary data. The expected usage model is:
 *
 * <ol>
 * <li>Compress the data by passing it to the RLECompressor constructor.</li>
 * <li>Repeatedly fetch a block of compressed data using the next(void *, std::size_t) function.</li>
 * <li>After each call to next(void *, std::size_t), call done() const to check whether all compressed data has been fetched or not.</li>
 * <li>Destroy the object.</li>
 * </ol>
 */
class RLECompressor {
	public:
		/**
		 * Constructs a new RLECompressor by compressing a block of binary data.
		 *
		 * \param[in] data the data to compress, which must not be freed until after the RLECompressor is destroyed.
		 *
		 * \param[in] length the length, in bytes, of \p data.
		 */
		RLECompressor(const void *data, std::size_t length);

		/**
		 * Checks whether all compressed data has already been fetched.
		 *
		 * \return \c true if calls to next(void *, std::size_t) have extracted all the RLE-compressed data,
		 * or \c false if there is more data to provide.
		 */
		bool done() const;

		/**
		 * Fetches the next block of compressed data.
		 * The output can be constrained to any size, but larger output block sizes will result in more efficient compression.
		 * In order to guarantee that progress will be made, the output block size must be at least three bytes.
		 *
		 * \param[out] buffer the buffer in which to store the compressed block.
		 *
		 * \param[in] length the length, in bytes, of \p buffer.
		 */
		std::size_t next(void *buffer, std::size_t length);

	private:
		class RLERun {
			public:
				RLERun(bool is_repeat, std::size_t length, const unsigned char *data, uint16_t crc = 0);
				bool done() const;
				std::size_t encode(void *buffer, std::size_t buflen);

			private:
				bool is_repeat;
				std::size_t length;
				const unsigned char *data;
				uint16_t crc;
		};

		std::vector<RLERun> runs;
		unsigned int cur_run;
};

#endif

