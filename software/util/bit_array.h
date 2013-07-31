#ifndef UTIL_BIT_ARRAY_H
#define UTIL_BIT_ARRAY_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

/**
 * \brief An array of bits that is also accessible as a C-style array of bytes.
 *
 * \tparam N the number of bits
 */
template<std::size_t N> class BitArray {
	public:
		/**
		 * \brief The data.
		 */
		std::array<uint8_t, (N + 7) / 8> bytes;

		/**
		 * \brief Constructs a new BitArray.
		 */
		BitArray() {
			zero();
		}

		/**
		 * \brief Gets the value of a bit.
		 *
		 * \param[in] i the index of the bit
		 *
		 * \return \c true if the bit is set, or \c false if it is cleared
		 */
		bool get(std::size_t i) const {
			assert(i < N);
			return !!(bytes[i / 8] & (1 << (i % 8)));
		}

		/**
		 * \brief Sets the value of a bit.
		 *
		 * \param[in] i the index of the bit
		 *
		 * \param[in] v the value to set the bit to
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
		 * \brief Sets all the bits to zero.
		 */
		void zero() {
			std::fill(bytes.begin(), bytes.end(), static_cast<uint8_t>(0));
		}
};

#endif

