#include "util/bitcodec_primitives.h"
#include <algorithm>
#include <stdint.h>
#include <vector>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

namespace {
	class DecodeTest : public CppUnit::TestFixture {
		CPPUNIT_TEST_SUITE(DecodeTest);
		CPPUNIT_TEST(test_byte);
		CPPUNIT_TEST(test_byte_shifted1);
		CPPUNIT_TEST(test_byte_shifted2);
		CPPUNIT_TEST(test_word);
		CPPUNIT_TEST(test_word_shifted);
		CPPUNIT_TEST(test_64_shifted);
		CPPUNIT_TEST(test_multiple);
		CPPUNIT_TEST_SUITE_END();

		public:
			void test_byte();
			void test_byte_shifted1();
			void test_byte_shifted2();
			void test_word();
			void test_word_shifted();
			void test_64_shifted();
			void test_multiple();
	};

	CPPUNIT_TEST_SUITE_REGISTRATION(DecodeTest);
}

void DecodeTest::test_byte() {
	static const uint8_t v[] = {
		0x5A, // 0b01011010
	};
	uint8_t u = BitcodecPrimitives::Decoder<uint8_t, 0, 8>() (v);
	CPPUNIT_ASSERT_EQUAL(0x5A, u + 0); // 0b01011010
}

void DecodeTest::test_byte_shifted1() {
	static const uint8_t v[] = {
		0x5A, // <padding × 1> || 0b1011010
	};
	uint8_t u = BitcodecPrimitives::Decoder<uint8_t, 1, 7>() (v);
	CPPUNIT_ASSERT_EQUAL(0x5A, u + 0); // 0b1011010
}

void DecodeTest::test_byte_shifted2() {
	static const uint8_t v[] = {
		0xBE, // 0b1011111 || <padding × 1>
	};
	uint8_t u = BitcodecPrimitives::Decoder<uint8_t, 0, 7>() (v);
	CPPUNIT_ASSERT_EQUAL(0x5F, u + 0); // 0b1011111
}

void DecodeTest::test_word() {
	static const uint8_t v[] = {
		0x12, // 0b00010010
		0x34, // 0b00110100
	};
	uint16_t u = BitcodecPrimitives::Decoder<uint16_t, 0, 16>() (v);
	CPPUNIT_ASSERT_EQUAL(0x1234, u + 0); // 0b0001001000110100
}

void DecodeTest::test_word_shifted() {
	static const uint8_t v[] = {
		0x00, // <padding × 8>
		0x02, // <padding × 5> || 0b010
		0x46, // 0b01000110
		0x80, // 0b100 || <padding × 5>
	};
	uint16_t u = BitcodecPrimitives::Decoder<uint16_t, 8 + 5, 14>() (v);
	CPPUNIT_ASSERT_EQUAL(0x1234, u + 0); // 0b01001000110100
}

void DecodeTest::test_64_shifted() {
	static const uint8_t v[] = {
		0x00, // <padding × 8>
		0x00, // <padding × 8>
		0x08, // <padding × 3> || 0b01000
		0xD1, // 0b11010001
		0x59, // 0b01011001
		0xE2, // 0b11100010
		0x6A, // 0b01101010
		0xF3, // 0b11110011
		0x7B, // 0b01111011
		0xFC, // 0b111111 || <padding × 2>
	};
	uint64_t u = BitcodecPrimitives::Decoder<uint64_t, 16 + 3, 59>() (v);
	CPPUNIT_ASSERT_EQUAL(UINT64_C(0x023456789ABCDEFF), u + 0); // 0b01000110100010101100111100010011010101111001101111011111111
}

void DecodeTest::test_multiple() {
	static const uint8_t v[] = {
		0x4D, // <padding × 1> || 0b1001101
		0x40, // 0b01 || <padding × 6>
	};
	uint8_t u1 = BitcodecPrimitives::Decoder<uint8_t, 1, 6>() (v);
	uint8_t u2 = BitcodecPrimitives::Decoder<uint8_t, 7, 3>() (v);
	CPPUNIT_ASSERT_EQUAL(0x26, u1 + 0); // 0b100110
	CPPUNIT_ASSERT_EQUAL(0x05, u2 + 0); // 0b101
}

