#include "util/codec.h"

namespace {
	/**
	 * Packs a sign bit, a biased exponent, and a significand into a 64-bit integer.
	 *
	 * \param[in] sign the sign bit, \c true for negative or \c false for positive.
	 *
	 * \param[in] exponent the biased exponent, between 0 and 0x7FF.
	 *
	 * \param[in] significand the significand.
	 */
	uint64_t pack_ses(bool sign, uint16_t exponent, uint64_t significand) {
		return (sign ? UINT64_C(0x8000000000000000) : 0) | (static_cast<uint64_t>(exponent) << 52) | significand;
	}
}

uint64_t Codec::double_to_u64(double x) {
	// Break down the number based on its coarse classification.
	int classify = std::fpclassify(x);
	if (classify == FP_NAN) {
		// NaN values are encoded by a biased exponent of 0x7FF and a nonzero significand.
		return pack_ses(false, 0x7FF, 1);
	} else if (classify == FP_INFINITE) {
		// Infinities are encoded by a biased exponent of 0x7FF and a zero significand.
		return pack_ses(!!std::signbit(x), 0x7FF, 0);
	} else if (classify == FP_ZERO || classify == FP_SUBNORMAL) {
		// Zeroes are encoded by a biased exponent of 0x000 and a zero significand.
		// I'm treating subnormals as zero.
		return pack_ses(!!std::signbit(x), 0x000, 0);
	} else {
		// Extract sign bit and absolutize input.
		bool sign = !!std::signbit(x);
		if (sign) {
			x = -x;
		}

		// Extract exponent and normalize input.
		int16_t exponent = 0;
		while (x >= 2.0) {
			x /= 2.0;
			++exponent;
		}
		while (x < 1.0) {
			x *= 2.0;
			--exponent;
		}

		// If exponent is below minimum limit, encode a zero.
		if (exponent < -1022) {
			return pack_ses(sign, 0x000, 0);
		}

		// If exponent is above maximum limit, encode an infinity.
		if (exponent > 1023) {
			return pack_ses(sign, 0x7FF, 0);
		}

		// The leading 1 bit of the significand is implied; remove it.
		x -= 1.0;

		// Encode the significand by multiplying the input by 2^52 and converting the result to an integer.
		uint64_t significand = static_cast<uint64_t>((x * (UINT64_C(1) << 52)) + 0.5);

		// Bias the exponent.
		exponent = static_cast<int16_t>(exponent + 1023);

		// Encode the number.
		return pack_ses(sign, exponent, significand);
	}
}

double Codec::u64_to_double(uint64_t x) {
	// Extract the sign bit, biased exponent, and significand.
	bool sign = !!(x & UINT64_C(0x8000000000000000));
	int16_t exponent = (x >> 52) & 0x7FF;
	uint64_t significand = x & UINT64_C(0x000FFFFFFFFFFFFF);

	// Break down the possible exponents by class.
	if (exponent == 0x7FF) {
		// Exponent 0x7FF encodes NaNs and infinities.
		if (significand) {
			return NAN;
		} else if (sign) {
			return -INFINITY;
		} else {
			return INFINITY;
		}
	} else if (!exponent) {
		// Exponent 0x000 encodes zeroes and subnormals.
		if (sign) {
			return -0.0;
		} else {
			return 0.0;
		}
	} else {
		// Unbias the exponent.
		exponent = static_cast<int16_t>(exponent - 1023);

		// Shift the significand down into the fraction part and re-add the implicit leading 1 bit.
		double value = 1.0 + static_cast<double>(significand) / (UINT64_C(1) << 52);

		// Apply the exponent.
		while (exponent > 0) {
			value *= 2.0;
			--exponent;
		}
		while (exponent < 0) {
			value /= 2.0;
			++exponent;
		}

		// Apply the sign bit.
		return sign ? -value : value;
	}
}

