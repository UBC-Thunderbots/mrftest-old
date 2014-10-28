#include "util/bitcodec_primitives.h"
#include <cinttypes>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#define BITCODEC_DEF_FILE "cppunit/util/bitcodec/complicated.def"
#define BITCODEC_STRUCT_NAME ComplicatedPacket
#define BITCODEC_ANON_NAMESPACE
#define BITCODEC_GEN_HEADER
#define BITCODEC_GEN_SOURCE
#include "util/bitcodec.h"
#undef BITCODEC_GEN_SOURCE
#undef BITCODEC_GEN_HEADER
#undef BITCODEC_DEF_FILE
#undef BITCODEC_STRUCT_NAME
#undef BITCODEC_ANON_NAMESPACE

namespace {
	class ComplicatedTest final : public CppUnit::TestFixture {
		CPPUNIT_TEST_SUITE(ComplicatedTest);
		CPPUNIT_TEST(test_default_construction);
		CPPUNIT_TEST(test_encoding);
		CPPUNIT_TEST(test_decoding);
		CPPUNIT_TEST(test_equality);
		CPPUNIT_TEST_SUITE_END();

		public:
			void test_default_construction();
			void test_encoding();
			void test_decoding();
			void test_equality();
	};

	CPPUNIT_TEST_SUITE_REGISTRATION(ComplicatedTest);
}

void ComplicatedTest::test_default_construction() {
	ComplicatedPacket pkt;
	CPPUNIT_ASSERT_EQUAL(false, pkt.flag1);
	CPPUNIT_ASSERT_EQUAL(true, pkt.flag2);
	CPPUNIT_ASSERT_EQUAL(false, pkt.flag3);
	CPPUNIT_ASSERT_EQUAL(static_cast<int16_t>(-2), pkt.tenbits1);
	CPPUNIT_ASSERT_EQUAL(static_cast<int16_t>(-1), pkt.tenbits2);
	CPPUNIT_ASSERT_EQUAL(static_cast<int16_t>(0), pkt.tenbits3);
	CPPUNIT_ASSERT_EQUAL(static_cast<int16_t>(507), pkt.tenbits4);
	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(12), pkt.little);
}

void ComplicatedTest::test_encoding() {
	ComplicatedPacket pkt;
	pkt.flag1 = true;
	pkt.flag2 = false;
	pkt.flag3 = true;
	pkt.tenbits1 = -27;
	pkt.tenbits2 = -400;
	pkt.tenbits3 = 20;
	pkt.tenbits4 = 475;
	pkt.little = 10;
	uint8_t buffer[ComplicatedPacket::BUFFER_SIZE];
	static_assert(sizeof(buffer) == 7, "Wrong buffer size generated!");
	std::fill(buffer, buffer + sizeof(buffer), 0xDB);
	pkt.encode(buffer);

	// Correct encoding is:
	//
	// +------+---------------+---------------+---------------+---------------+---------------+---------------+---------------+
	// | Byte |       0       |       1       |       2       |       3       |       4       |       5       |       6       |
	// | Bit  |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
	// +------+---------------+---------------+---------------+---------------+---------------+---------------+---------------+
	// |Flag1 |1              |               |               |               |               |               |               |
	// |Flag2 |  0            |               |               |               |               |               |               |
	// |Flag3 |      1        |               |               |               |               |               |               |
	// | 10b1 |               |1 1 1 1 1 0 0 1|0 1            |               |               |               |               |
	// | 10b2 |               |               |    1 0 0 1 1 1|0 0 0 0        |               |               |               |
	// | 10b3 |               |               |               |        0 0 0 0|0 1 0 1 0 0    |               |               |
	// | 10b4 |               |               |               |               |            0 1|1 1 0 1 1 0 1 1|               |
	// |little|               |               |               |               |               |               |1 0 1 0        |
	// | Pad  |    0   0 0 0 0|               |               |               |               |               |        0 0 0 0|
	// +------+---------------+---------------+---------------+---------------+---------------+---------------+---------------+
	// | Bits |1 0 0 1 0 0 0 0|1 1 1 1 1 0 0 1|0 1 1 0 0 1 1 1|0 0 0 0 0 0 0 0|0 1 0 1 0 0 0 1|1 1 0 1 1 0 1 1|1 0 1 0 0 0 0 0|
	// | Hex  |   9       0   |   F       9   |   6       7   |   0       0   |   5       1   |   D       B   |   A       0   |
	// +------+---------------+---------------+---------------+---------------+---------------+---------------+---------------+

	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0x90), buffer[0]);
	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0xF9), buffer[1]);
	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0x67), buffer[2]);
	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0x00), buffer[3]);
	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0x51), buffer[4]);
	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0xDB), buffer[5]);
	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0xA0), buffer[6]);
}

void ComplicatedTest::test_decoding() {
	static const uint8_t buffer[7] = { 0x90, 0xF9, 0x67, 0x00, 0x51, 0xDB, 0xA0 };
	const ComplicatedPacket pkt(buffer);
	CPPUNIT_ASSERT_EQUAL(true, pkt.flag1);
	CPPUNIT_ASSERT_EQUAL(false, pkt.flag2);
	CPPUNIT_ASSERT_EQUAL(true, pkt.flag3);
	CPPUNIT_ASSERT_EQUAL(static_cast<int16_t>(-27), pkt.tenbits1);
	CPPUNIT_ASSERT_EQUAL(static_cast<int16_t>(-400), pkt.tenbits2);
	CPPUNIT_ASSERT_EQUAL(static_cast<int16_t>(20), pkt.tenbits3);
	CPPUNIT_ASSERT_EQUAL(static_cast<int16_t>(475), pkt.tenbits4);
	CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(10), pkt.little);
}

void ComplicatedTest::test_equality() {
	ComplicatedPacket pkt1;
	pkt1.flag1 = true;
	pkt1.flag2 = false;
	pkt1.flag3 = true;
	pkt1.tenbits1 = -27;
	pkt1.tenbits2 = -400;
	pkt1.tenbits3 = 20;
	pkt1.tenbits4 = 475;
	pkt1.little = 10;

	ComplicatedPacket pkt2 = pkt1;

	CPPUNIT_ASSERT(pkt1 == pkt2);

	++pkt1.little;

	CPPUNIT_ASSERT(pkt1 != pkt2);
}

