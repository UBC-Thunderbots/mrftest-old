#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include "util/bitcodec_primitives.h"

namespace
{
class EncodeTest final : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(EncodeTest);
    CPPUNIT_TEST(test_byte);
    CPPUNIT_TEST(test_byte_shifted1);
    CPPUNIT_TEST(test_byte_shifted2);
    CPPUNIT_TEST(test_word);
    CPPUNIT_TEST(test_word_shifted);
    CPPUNIT_TEST(test_64_shifted);
    CPPUNIT_TEST(test_multiple);
    CPPUNIT_TEST_SUITE_END();

   public:
    void setUp();
    void test_byte();
    void test_byte_shifted1();
    void test_byte_shifted2();
    void test_word();
    void test_word_shifted();
    void test_64_shifted();
    void test_multiple();

   private:
    uint8_t buffer[64];

    void check_last_buffer_bytes_zero(std::size_t num);
};

CPPUNIT_TEST_SUITE_REGISTRATION(EncodeTest);
}

void EncodeTest::setUp()
{
    std::fill(buffer, buffer + sizeof(buffer), 0);
}

void EncodeTest::test_byte()
{
    BitcodecPrimitives::Encoder<uint8_t, 0, 8>()(buffer, 0x5A);  // 0b01011010
    CPPUNIT_ASSERT_EQUAL(0x5A, buffer[0] + 0);                   // 0b01011010
    check_last_buffer_bytes_zero(sizeof(buffer) - 1);
}

void EncodeTest::test_byte_shifted1()
{
    BitcodecPrimitives::Encoder<uint8_t, 1, 7>()(buffer, 0x5A);  // 0b1011010
    CPPUNIT_ASSERT_EQUAL(0x5A, buffer[0] + 0);  // <padding × 1> || 0b1011010
    check_last_buffer_bytes_zero(sizeof(buffer) - 1);
}

void EncodeTest::test_byte_shifted2()
{
    BitcodecPrimitives::Encoder<uint8_t, 0, 7>()(buffer, 0x5F);  // 0b1011111
    CPPUNIT_ASSERT_EQUAL(0xBE, buffer[0] + 0);  // 0b1011111 || <padding × 1>
    check_last_buffer_bytes_zero(sizeof(buffer) - 1);
}

void EncodeTest::test_word()
{
    BitcodecPrimitives::Encoder<uint16_t, 0, 16>()(
        buffer, 0x1234);                        // 0b0001001000110100
    CPPUNIT_ASSERT_EQUAL(0x12, buffer[0] + 0);  // 0b00010010
    CPPUNIT_ASSERT_EQUAL(0x34, buffer[1] + 0);  // 0b00110100
    check_last_buffer_bytes_zero(sizeof(buffer) - 2);
}

void EncodeTest::test_word_shifted()
{
    BitcodecPrimitives::Encoder<uint16_t, 8 + 5, 14>()(
        buffer, 0x1234);                        // 0b01001000110100
    CPPUNIT_ASSERT_EQUAL(0x00, buffer[0] + 0);  // <padding × 8>
    CPPUNIT_ASSERT_EQUAL(0x02, buffer[1] + 0);  // <padding × 5> || 0b010
    CPPUNIT_ASSERT_EQUAL(0x46, buffer[2] + 0);  // 0b01000110
    CPPUNIT_ASSERT_EQUAL(0x80, buffer[3] + 0);  // 0b100 || <padding × 5>
    check_last_buffer_bytes_zero(sizeof(buffer) - 4);
}

void EncodeTest::test_64_shifted()
{
    // 0b01000110100010101100111100010011010101111001101111011111111
    BitcodecPrimitives::Encoder<uint64_t, 16 + 3, 59>()(
        buffer, UINT64_C(0x023456789ABCDEFF));
    CPPUNIT_ASSERT_EQUAL(0x00, buffer[0] + 0);  // <padding × 8>
    CPPUNIT_ASSERT_EQUAL(0x00, buffer[1] + 0);  // <padding × 8>
    CPPUNIT_ASSERT_EQUAL(0x08, buffer[2] + 0);  // <padding × 3> || 0b01000
    CPPUNIT_ASSERT_EQUAL(0xD1, buffer[3] + 0);  // 0b11010001
    CPPUNIT_ASSERT_EQUAL(0x59, buffer[4] + 0);  // 0b01011001
    CPPUNIT_ASSERT_EQUAL(0xE2, buffer[5] + 0);  // 0b11100010
    CPPUNIT_ASSERT_EQUAL(0x6A, buffer[6] + 0);  // 0b01101010
    CPPUNIT_ASSERT_EQUAL(0xF3, buffer[7] + 0);  // 0b11110011
    CPPUNIT_ASSERT_EQUAL(0x7B, buffer[8] + 0);  // 0b01111011
    CPPUNIT_ASSERT_EQUAL(0xFC, buffer[9] + 0);  // 0b111111 || <padding × 2>
    check_last_buffer_bytes_zero(sizeof(buffer) - 10);
}

void EncodeTest::test_multiple()
{
    BitcodecPrimitives::Encoder<uint8_t, 1, 6>()(buffer, 0x26);  // 0b100110
    BitcodecPrimitives::Encoder<uint8_t, 7, 3>()(buffer, 0x05);  // 0b101
    CPPUNIT_ASSERT_EQUAL(0x4D, buffer[0] + 0);  // <padding × 1> || 0b1001101
    CPPUNIT_ASSERT_EQUAL(0x40, buffer[1] + 0);  // 0b01 || <padding × 6>
    check_last_buffer_bytes_zero(sizeof(buffer) - 2);
}

void EncodeTest::check_last_buffer_bytes_zero(std::size_t num)
{
    for (std::size_t i = sizeof(buffer) - num; i != sizeof(buffer); ++i)
    {
        CPPUNIT_ASSERT_EQUAL(0x00, buffer[i] + 0);
    }
}
