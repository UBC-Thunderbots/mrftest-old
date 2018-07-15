#include <gtest/gtest.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include "util/bitcodec_primitives.h"

namespace
{
class EncodeTest : public ::testing::Test
{
   protected:
    virtual void SetUp()
    {
        std::fill(buffer, buffer + sizeof(buffer), 0);
    }

    uint8_t buffer[64];

    void check_last_buffer_bytes_zero(std::size_t num);
};

TEST_F(EncodeTest, test_byte)
{
    BitcodecPrimitives::Encoder<uint8_t, 0, 8>()(buffer, 0x5A);  // 0b01011010
    EXPECT_EQ(0x5A, buffer[0] + 0);                              // 0b01011010
    check_last_buffer_bytes_zero(sizeof(buffer) - 1);
}

TEST_F(EncodeTest, test_byte_shifted1)
{
    BitcodecPrimitives::Encoder<uint8_t, 1, 7>()(buffer, 0x5A);  // 0b1011010
    EXPECT_EQ(0x5A, buffer[0] + 0);  // <padding × 1> || 0b1011010
    check_last_buffer_bytes_zero(sizeof(buffer) - 1);
}

TEST_F(EncodeTest, test_byte_shifted2)
{
    BitcodecPrimitives::Encoder<uint8_t, 0, 7>()(buffer, 0x5F);  // 0b1011111
    EXPECT_EQ(0xBE, buffer[0] + 0);  // 0b1011111 || <padding × 1>
    check_last_buffer_bytes_zero(sizeof(buffer) - 1);
}

TEST_F(EncodeTest, test_word)
{
    BitcodecPrimitives::Encoder<uint16_t, 0, 16>()(
        buffer, 0x1234);             // 0b0001001000110100
    EXPECT_EQ(0x12, buffer[0] + 0);  // 0b00010010
    EXPECT_EQ(0x34, buffer[1] + 0);  // 0b00110100
    check_last_buffer_bytes_zero(sizeof(buffer) - 2);
}

TEST_F(EncodeTest, test_word_shifted)
{
    BitcodecPrimitives::Encoder<uint16_t, 8 + 5, 14>()(
        buffer, 0x1234);             // 0b01001000110100
    EXPECT_EQ(0x00, buffer[0] + 0);  // <padding × 8>
    EXPECT_EQ(0x02, buffer[1] + 0);  // <padding × 5> || 0b010
    EXPECT_EQ(0x46, buffer[2] + 0);  // 0b01000110
    EXPECT_EQ(0x80, buffer[3] + 0);  // 0b100 || <padding × 5>
    check_last_buffer_bytes_zero(sizeof(buffer) - 4);
}

TEST_F(EncodeTest, test_64_shifted)
{
    // 0b01000110100010101100111100010011010101111001101111011111111
    BitcodecPrimitives::Encoder<uint64_t, 16 + 3, 59>()(
        buffer, UINT64_C(0x023456789ABCDEFF));
    EXPECT_EQ(0x00, buffer[0] + 0);  // <padding × 8>
    EXPECT_EQ(0x00, buffer[1] + 0);  // <padding × 8>
    EXPECT_EQ(0x08, buffer[2] + 0);  // <padding × 3> || 0b01000
    EXPECT_EQ(0xD1, buffer[3] + 0);  // 0b11010001
    EXPECT_EQ(0x59, buffer[4] + 0);  // 0b01011001
    EXPECT_EQ(0xE2, buffer[5] + 0);  // 0b11100010
    EXPECT_EQ(0x6A, buffer[6] + 0);  // 0b01101010
    EXPECT_EQ(0xF3, buffer[7] + 0);  // 0b11110011
    EXPECT_EQ(0x7B, buffer[8] + 0);  // 0b01111011
    EXPECT_EQ(0xFC, buffer[9] + 0);  // 0b111111 || <padding × 2>
    check_last_buffer_bytes_zero(sizeof(buffer) - 10);
}

TEST_F(EncodeTest, test_multiple)
{
    BitcodecPrimitives::Encoder<uint8_t, 1, 6>()(buffer, 0x26);  // 0b100110
    BitcodecPrimitives::Encoder<uint8_t, 7, 3>()(buffer, 0x05);  // 0b101
    EXPECT_EQ(0x4D, buffer[0] + 0);  // <padding × 1> || 0b1001101
    EXPECT_EQ(0x40, buffer[1] + 0);  // 0b01 || <padding × 6>
    check_last_buffer_bytes_zero(sizeof(buffer) - 2);
}

void EncodeTest::check_last_buffer_bytes_zero(std::size_t num)
{
    for (std::size_t i = sizeof(buffer) - num; i != sizeof(buffer); ++i)
    {
        EXPECT_EQ(0x00, buffer[i] + 0);
    }
}

}  // namespace
