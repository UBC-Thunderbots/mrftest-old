#include <gtest/gtest.h>
#include <cinttypes>
#include "util/bitcodec_primitives.h"

#define BITCODEC_DEF_FILE "test/unit-tests/util/bitcodec/simple.def"
#define BITCODEC_STRUCT_NAME SimplePacket
#define BITCODEC_ANON_NAMESPACE
#define BITCODEC_GEN_HEADER
#define BITCODEC_GEN_SOURCE
#include "util/bitcodec.h"
#undef BITCODEC_GEN_SOURCE
#undef BITCODEC_GEN_HEADER
#undef BITCODEC_DEF_FILE
#undef BITCODEC_STRUCT_NAME
#undef BITCODEC_ANON_NAMESPACE

namespace
{
TEST(SimpleTest, test_default_construction)
{
    SimplePacket pkt;
    EXPECT_EQ(static_cast<uint8_t>(0), pkt.byte1);
    EXPECT_EQ(static_cast<uint8_t>(27), pkt.byte2);
    EXPECT_EQ(static_cast<uint32_t>(1234), pkt.dword);
}

TEST(SimpleTest, test_encoding)
{
    SimplePacket pkt;
    pkt.byte1 = 0x42;
    pkt.byte2 = 0x90;
    pkt.dword = UINT32_C(0x12345678);
    uint8_t buffer[SimplePacket::BUFFER_SIZE];
    static_assert(sizeof(buffer) == 6, "Wrong buffer size generated!");
    std::fill(buffer, buffer + sizeof(buffer), 0xDB);
    pkt.encode(buffer);
    EXPECT_EQ(static_cast<uint8_t>(0x42), buffer[0]);
    EXPECT_EQ(static_cast<uint8_t>(0x90), buffer[1]);
    EXPECT_EQ(static_cast<uint8_t>(0x12), buffer[2]);
    EXPECT_EQ(static_cast<uint8_t>(0x34), buffer[3]);
    EXPECT_EQ(static_cast<uint8_t>(0x56), buffer[4]);
    EXPECT_EQ(static_cast<uint8_t>(0x78), buffer[5]);
}

TEST(SimpleTest, test_decoding)
{
    static const uint8_t buffer[SimplePacket::BUFFER_SIZE] = {0x42, 0x90, 0x12,
                                                              0x34, 0x56, 0x78};
    const SimplePacket pkt(buffer);
    EXPECT_EQ(static_cast<uint8_t>(0x42), pkt.byte1);
    EXPECT_EQ(static_cast<uint8_t>(0x90), pkt.byte2);
    EXPECT_EQ(static_cast<uint32_t>(0x12345678), pkt.dword);
}

TEST(SimpleTest, test_equality)
{
    SimplePacket pkt1;
    pkt1.byte1 = 0x42;
    pkt1.byte2 = 0x90;
    pkt1.dword = UINT32_C(0x12345678);

    SimplePacket pkt2;
    pkt2.byte1 = 0x42;
    pkt2.byte2 = 0x90;
    pkt2.dword = UINT32_C(0x12345678);

    EXPECT_TRUE(pkt1 == pkt2);

    ++pkt2.byte2;

    EXPECT_TRUE(pkt1 != pkt2);
}

}  // namespace
