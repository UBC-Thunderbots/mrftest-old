#ifndef UTIL_BIT_ARRAY_H
#define UTIL_BIT_ARRAY_H

#include <cassert>
#include <cstddef>
#include <stdint.h>

/**
 * An array of bits that is also accessible as a C-style array of bytes.
 *
 * \tparam N the number of bits.
 */
template<std::size_t N>
class BitArray {
	public:
		/**
		 * The data.
		 */
		uint8_t bytes[(N + 7) / 8];

		/**
		 * Constructs a new BitArray.
		 */
		BitArray() {
			zero();
		}

		/**
		 * Gets the value of a bit.
		 *
		 * \param[in] i the index of the bit.
		 */
		bool get(std::size_t i) const {
			assert(i < N);
			return !!(bytes[i / 8] & (1 << (i % 8)));
		}

		/**
		 * Sets the value of a bit.
		 *
		 * \param[in] i the index of the bit.
		 *
		 * \param[in] v the value to set the bit to.
		 */
		void set(std::size_t i, bool v) {
			assert(i < N);
			if (v) {
				bytes[i / 8] |= static_cast<uint8_t>(1 << (i % 8));
			} else {
				bytes[i / 8] &= static_cast<uint8_t>(~(1 << (i % 8)));
			}
		}

		/**
		 * Sets all the bits to zero.
		 */
		void zero() {
			for (std::size_t i = 0; i < sizeof(bytes); ++i) {
				bytes[i] = 0;
			}
		}

		/**
		 * Compares two BitArrays for equality.
		 *
		 * \param[in] other the array to compare.
		 *
		 * \return \c true if the contents of the arrays are the same, or \c false if not.
		 */
		bool operator==(const BitArray<N> &other) const {
			for (std::size_t i = 0; i < sizeof(bytes); ++i) {
				if (bytes[i] != other.bytes[i]) {
					return false;
				}
			}
			return true;
		}

		/**
		 * Compares two BitArrays for inequality.
		 *
		 * \param[in] other the array to compare.
		 *
		 * \return \c false if the contents of the arrays are the same, or \c true if not.
		 */
		bool operator!=(const BitArray<N> &other) const {
			for (std::size_t i = 0; i < sizeof(bytes); ++i) {
				if (bytes[i] != other.bytes[i]) {
					return true;
				}
			}
			return false;
		}
};

#endif

