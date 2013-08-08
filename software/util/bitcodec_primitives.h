#ifndef UTIL_BITCODEC_PRIMITIVES_H
#define UTIL_BITCODEC_PRIMITIVES_H

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace BitcodecPrimitives {
	/**
	 * \brief Encodes a single field into selected bits of a buffer.
	 *
	 * \tparam T the type of the field, which must be an unsigned integral type.
	 *
	 * \tparam Offset the offset into the buffer at which to store the field, measured in bits.
	 *
	 * \tparam Length the number of bits to use to store the field.
	 */
	template<typename T, std::size_t Offset, std::size_t Length> class Encoder {
		public:
			/**
			 * \brief Executes the encoding operation.
			 *
			 * \param[out] buffer the buffer to store the field into.
			 *
			 * \param[in] value the field value to store.
			 */
			void operator()(void *buffer, T value) const {
				uint8_t *p = static_cast<uint8_t *>(buffer) + (Offset / 8);
				*p = static_cast<uint8_t>(*p | (((value >> VALUE_SHIFT) & MASK) << (8 - (BITS_THIS + (Offset % 8)))));
				Encoder<T, 0, Length - BITS_THIS>() (p + 1, value);
			}

		private:
			static_assert(sizeof(T) <= 8, "value must be no larger than a uint64_t!");
			static_assert(sizeof(T) * 8 >= Length, "value must be large enough to hold \"Length\" bits!");
			static_assert(Length, "Length must be nonzero!");

			static constexpr std::size_t BITS_THIS = Length < 8 - (Offset % 8) ? Length : 8 - (Offset % 8);
			static constexpr std::size_t VALUE_SHIFT = Length - BITS_THIS;
			static constexpr T MASK = (static_cast<T>(1U) << BITS_THIS) - static_cast<T>(1U);
	};

	template<typename T, std::size_t Offset, std::size_t Length> constexpr std::size_t Encoder<T, Offset, Length>::BITS_THIS;
	template<typename T, std::size_t Offset, std::size_t Length> constexpr std::size_t Encoder<T, Offset, Length>::VALUE_SHIFT;
	template<typename T, std::size_t Offset, std::size_t Length> constexpr T Encoder<T, Offset, Length>::MASK;

	/** \cond */
	template<typename T, std::size_t Offset> class Encoder<T, Offset, 0> {
		public:
			void operator()(void *, T) const {
			}
	};
	/** \endcond */

	/**
	 * \brief Extracts and decodes a single field from selected bits of a buffer.
	 *
	 * \tparam T the type of the field, which must be an unsigned integral type.
	 *
	 * \tparam Offset the offset into the buffer at which to extract the field, measured in bits.
	 *
	 * \tparam Length the number of bits to extract from the buffer.
	 */
	template<typename T, std::size_t Offset, std::size_t Length> class Decoder {
		public:
			/**
			 * \brief Executes the decoding operation.
			 *
			 * \param[in] buffer the buffer to extract the field from.
			 *
			 * \return the field value.
			 */
			T operator()(const void *buffer) const {
				const uint8_t *p = static_cast<const uint8_t *>(buffer) + (Offset / 8);
				T tmp = static_cast<T>(((static_cast<T>(*p) >> (8 - (BITS_THIS + (Offset % 8)))) & MASK) << (Length - BITS_THIS));
				return tmp | Decoder<T, 0, Length - BITS_THIS>() (p + 1);
			}

		private:
			static_assert(sizeof(T) <= 8, "value must be no larger than a uint64_t!");
			static_assert(sizeof(T) * 8 >= Length, "value must be large enough to hold \"Length\" bits!");
			static_assert(Length, "Length must be nonzero!");

			static constexpr std::size_t BITS_THIS = Length < 8 - (Offset % 8) ? Length : 8 - (Offset % 8);
			static constexpr T MASK = (static_cast<T>(1U) << BITS_THIS) - static_cast<T>(1U);
	};

	template<typename T, std::size_t Offset, std::size_t Length> constexpr std::size_t Decoder<T, Offset, Length>::BITS_THIS;
	template<typename T, std::size_t Offset, std::size_t Length> constexpr T Decoder<T, Offset, Length>::MASK;

	/** \cond */
	template<typename T, std::size_t Offset> class Decoder<T, Offset, 0> {
		public:
			T operator()(const void *) const {
				return 0;
			}
	};
	/** \endcond */

	template<std::size_t ... Elements> struct OverlapChecker;

	/**
	 * \brief Checks, at compile time, for overlap among an ascending sequence of intervals.
	 *
	 * \tparam Offset1 the start position of the first interval.
	 *
	 * \tparam Length1 the size of the first interval.
	 *
	 * \tparam Offset2 the start position of the second interval.
	 *
	 * \tparam Length2 the size of the second interval.
	 *
	 * \tparam Tail the third and subsequent intervals, each consisting of a position followed by a length.
	 */
	template<std::size_t Offset1, std::size_t Length1, std::size_t Offset2, std::size_t Length2, std::size_t ... Tail> struct OverlapChecker<Offset1, Length1, Offset2, Length2, Tail ...> {
		/**
		 * \brief \c true if none of the intervals overlap, or \c false if some intervals do overlap.
		 */
		static constexpr bool OK = (Offset2 >= Offset1 + Length1) && OverlapChecker<Offset2, Length2, Tail ...>::OK;
	};

	template<std::size_t Offset1, std::size_t Length1, std::size_t Offset2, std::size_t Length2, std::size_t ... Tail> constexpr bool OverlapChecker<Offset1, Length1, Offset2, Length2, Tail ...>::OK;

	/**
	 * \brief Checks, at compile time, for overlap among an ascending sequence of intervals.
	 *
	 * \tparam Offset the start position of the only interval.
	 *
	 * \tparam Length the size of the only interval.
	 */
	template<std::size_t Offset, std::size_t Length> struct OverlapChecker<Offset, Length> {
		/**
		 * \brief \c true, because a single interval cannot have an overlap.
		 */
		static constexpr bool OK = true;
	};

	template<std::size_t Offset, std::size_t Length> constexpr bool OverlapChecker<Offset, Length>::OK;

	/**
	 * \brief Checks, at compile time, for overlap among an ascending sequence of intervals.
	 */
	template<> struct OverlapChecker<> {
		/**
		 * \brief \c true, because a list of zero intervals cannot have an overlap.
		 */
		static constexpr bool OK = true;
	};

	template<std::size_t ... Elements> struct LengthCalculator;

	/**
	 * \brief Computes, at compile time, the length in bytes of the buffer required to store a sequence of bitfield elements.
	 *
	 * \tparam Offset the starting position of the first element.
	 *
	 * \tparam Length the number of bits used by the first element.
	 *
	 * \tparam Tail the remaining elements, each consiting of a starting offset followed by a length.
	 */
	template<std::size_t Offset, std::size_t Length, std::size_t ... Tail> struct LengthCalculator<Offset, Length, Tail ...> {
		/**
		 * \brief The number of bytes needed to hold all the elements.
		 */
		static constexpr std::size_t BYTES = LengthCalculator<Tail ...>::BYTES;
	};

	template<std::size_t Offset, std::size_t Length, std::size_t ... Tail> constexpr std::size_t LengthCalculator<Offset, Length, Tail ...>::BYTES;

	/**
	 * \brief Computes, at compile time, the length in bytes of the buffer required to store a sequence of bitfield elements.
	 *
	 * \tparam Offset the starting position of the only element.
	 *
	 * \tparam Length the number of bits used by the only first element.
	 */
	template<std::size_t Offset, std::size_t Length> struct LengthCalculator<Offset, Length> {
		/**
		 * \brief The number of bytes needed to hold the element.
		 */
		static constexpr std::size_t BYTES = (Offset + Length + 7) / 8;
	};

	template<std::size_t Offset, std::size_t Length> constexpr std::size_t LengthCalculator<Offset, Length>::BYTES;

	/**
	 * \brief Computes, at compile time, the length in bytes of the buffer required to store a sequence of bitfield elements.
	 */
	template<> struct LengthCalculator<> {
		/**
		 * \brief 0, because a sequence of no elements takes no space to store.
		 */
		static constexpr std::size_t BYTES = 0;
	};

	/**
	 * \brief Transforms an unsigned integral type into the corresponding signed type by doing sign extension.
	 *
	 * \tparam T the signed type to transform the value into.
	 *
	 * \tparam U the unsigned type in which the value is initially represented.
	 *
	 * \tparam Length the number of bits of \p U which are used to represent the value.
	 */
	template<typename T, typename U, std::size_t Length> struct SignExtender {
		/**
		 * \brief Performs sign extensions.
		 *
		 * \param[in] x the value to convert.
		 *
		 * \return the converted value.
		 */
		constexpr T operator()(U x) const {
			return (x & static_cast<U>(static_cast<U>(1) << (Length - 1))) ? static_cast<T>(x | static_cast<U>(static_cast<U>(-1) << Length)) : static_cast<T>(x);
		}
	};
}

#endif

