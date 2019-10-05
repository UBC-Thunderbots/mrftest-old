#include <gtest/gtest.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include "util/bitcodec_primitives.h"

namespace
{
TEST(DecodeTest, test_byte)
{
    static const uint8_t v[] = {
        0x5A,  // 0b01011010
    };
    uint8_t u = BitcodecPrimitives::Decoder<uint8_t, 0, 8>()(v);
    EXPECT_EQ(0x5A, u + 0);  // 0b01011010
}

TEST(DecodeTest, test_byte_shifted1)
{
    static const uint8_t v[] = {
        0x5A,  // <padding × 1> || 0b1011010
    };
    uint8_t u = BitcodecPrimitives::Decoder<uint8_t, 1, 7>()(v);
    EXPECT_EQ(0x5A, u + 0);  // 0b1011010
}

TEST(DecodeTest, test_byte_shifted2)
{
    static const uint8_t v[] = {
        0xBE,  // 0b1011111 || <padding × 1>
    };
    uint8_t u = BitcodecPrimitives::Decoder<uint8_t, 0, 7>()(v);
    EXPECT_EQ(0x5F, u + 0);  // 0b1011111
}

TEST(DecodeTest, test_word)
{
    static const uint8_t v[] = {
        0x12,  // 0b00010010
        0x34,  // 0b00110100
    };
    uint16_t u = BitcodecPrimitives::Decoder<uint16_t, 0, 16>()(v);
    EXPECT_EQ(0x1234, u + 0);  // 0b0001001000110100
}

TEST(DecodeTest, test_word_shifted)
{
    static const uint8_t v[] = {
        0x00,  // <padding × 8>
        0x02,  // <padding × 5> || 0b010
        0x46,  // 0b01000110
        0x80,  // 0b100 || <padding × 5>
    };
    uint16_t u = BitcodecPrimitives::Decoder<uint16_t, 8 + 5, 14>()(v);
    EXPECT_EQ(0x1234, u + 0);  // 0b01001000110100
}

TEST(DecodeTest, test_64_shifted)
{
    static const uint8_t v[] = {
        0x00,  // <padding × 8>
        0x00,  // <padding × 8>
        0x08,  // <padding × 3> || 0b01000
        0xD1,  // 0b11010001
        0x59,  // 0b01011001
        0xE2,  // 0b11100010
        0x6A,  // 0b01101010
        0xF3,  // 0b11110011
        0x7B,  // 0b01111011
        0xFC,  // 0b111111 || <padding × 2>
    };
    uint64_t u = BitcodecPrimitives::Decoder<uint64_t, 16 + 3, 59>()(v);
    EXPECT_EQ(
        UINT64_C(0x023456789ABCDEFF),
        u + 0);  // 0b01000110100010101100111100010011010101111001101111011111111
}

TEST(DecodeTest, test_multiple)
{
    static const uint8_t v[] = {
        0x4D,  // <padding × 1> || 0b1001101
        0x40,  // 0b01 || <padding × 6>
    };
    uint8_t u1 = BitcodecPrimitives::Decoder<uint8_t, 1, 6>()(v);
    uint8_t u2 = BitcodecPrimitives::Decoder<uint8_t, 7, 3>()(v);
    EXPECT_EQ(0x26, u1 + 0);  // 0b100110
    EXPECT_EQ(0x05, u2 + 0);  // 0b101
}

}  // namespace
