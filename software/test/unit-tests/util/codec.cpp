#include "util/codec.h"
#include <gtest/gtest.h>
#include <cmath>

namespace
{
TEST(CodecTest, test_zeroes)
{
    EXPECT_EQ(0.0, decode_u64_to_double(0));
    EXPECT_EQ(-0.0, decode_u64_to_double(UINT64_C(0x8000000000000000)));
    EXPECT_EQ(UINT64_C(0), encode_double_to_u64(0.0));
    EXPECT_EQ(UINT64_C(0x8000000000000000), encode_double_to_u64(-0.0));
    EXPECT_EQ(0.0f, decode_u32_to_float(0));
    EXPECT_EQ(-0.0f, decode_u32_to_float(UINT32_C(0x80000000)));
    EXPECT_EQ(UINT32_C(0), encode_float_to_u32(0.0f));
    EXPECT_EQ(UINT32_C(0x80000000), encode_float_to_u32(-0.0f));
}

TEST(CodecTest, test_nans)
{
    EXPECT_TRUE(std::isnan(decode_u64_to_double(UINT64_C(0x7FF0000000000001))));
    EXPECT_EQ(
        UINT64_C(0x7FF0000000000000),
        encode_double_to_u64(NAN) & UINT64_C(0x7FF0000000000000));
    EXPECT_TRUE(
        (encode_double_to_u64(NAN) & UINT64_C(0x000FFFFFFFFFFFFF)) != 0);
    EXPECT_TRUE(std::isnan(decode_u32_to_float(UINT32_C(0x7F800001))));
    EXPECT_EQ(
        UINT32_C(0x7F800000), encode_float_to_u32(NAN) & UINT32_C(0x7F800000));
    EXPECT_TRUE((encode_float_to_u32(NAN) & UINT32_C(0x007FFFFF)) != 0);
}

TEST(CodecTest, test_infinities)
{
    EXPECT_TRUE(std::isinf(decode_u64_to_double(UINT64_C(0x7FF0000000000000))));
    EXPECT_TRUE(decode_u64_to_double(UINT64_C(0x7FF0000000000000)) > 0);
    EXPECT_EQ(UINT64_C(0x7FF0000000000000), encode_double_to_u64(INFINITY));
    EXPECT_TRUE(std::isinf(decode_u64_to_double(UINT64_C(0xFFF0000000000000))));
    EXPECT_TRUE(decode_u64_to_double(UINT64_C(0xFFF0000000000000)) < 0);
    EXPECT_EQ(UINT64_C(0xFFF0000000000000), encode_double_to_u64(-INFINITY));
    EXPECT_TRUE(std::isinf(decode_u32_to_float(UINT32_C(0x7F800000))));
    EXPECT_TRUE(decode_u32_to_float(UINT32_C(0x7F800000)) > 0);
    EXPECT_EQ(UINT32_C(0x7F800000), encode_float_to_u32(INFINITY));
    EXPECT_TRUE(std::isinf(decode_u32_to_float(UINT32_C(0xFF800000))));
    EXPECT_TRUE(decode_u32_to_float(UINT32_C(0xFF800000)) < 0);
    EXPECT_EQ(UINT32_C(0xFF800000), encode_float_to_u32(-INFINITY));
}

TEST(CodecTest, test_normals)
{
    static const struct
    {
        double d;
        uint64_t u64;
    } DOUBLE_VALUES[] = {
        {1.0, (UINT64_C(0) << 63) | (UINT64_C(0x3FF) << 52) | UINT64_C(0)},
        {-256.0000000038045300243538804352283477783203125,
         (UINT64_C(1) << 63) | (UINT64_C(0x407) << 52) | UINT64_C(66930)},
        {0.12500129187120634366436888740281574428081512451171875,
         (UINT64_C(0) << 63) | (UINT64_C(0x3FC) << 52) | UINT64_C(46544565468)},
    };
    static const struct
    {
        float fl;
        uint32_t u32;
    } FLOAT_VALUES[] = {
        {1.0f, (UINT32_C(0) << 31) | (UINT32_C(0x7F) << 23) | UINT32_C(0)},
        {-75.8015594482421875f,
         (UINT32_C(1) << 31) | (UINT32_C(0x85) << 23) | UINT32_C(1546854)},
        {0.037382401525974273681640625f,
         (UINT32_C(0) << 31) | (UINT32_C(0x7A) << 23) | UINT32_C(1646154)},
    };

    for (const auto &i : DOUBLE_VALUES)
    {
        EXPECT_EQ(i.u64, encode_double_to_u64(i.d));
        EXPECT_EQ(i.d, decode_u64_to_double(i.u64));
    }

    for (const auto &i : FLOAT_VALUES)
    {
        EXPECT_EQ(i.u32, encode_float_to_u32(i.fl));
        EXPECT_EQ(i.fl, decode_u32_to_float(i.u32));
    }
}

TEST(CodecTest, test_subnormals)
{
    static const struct
    {
        double d;
        uint64_t u64;
    } DOUBLE_VALUES[] = {
        {0.0, (UINT64_C(0) << 63) | (UINT64_C(0) << 52) | UINT64_C(0)},
        {0.00000000000000000000000000000000000000000000270015986022750548978769e-271,
         (UINT64_C(0) << 63) | (UINT64_C(0) << 52) | UINT64_C(54651844)},
        {-0.00000000000000000000000000000000000000000000270015986022750548978769e-271,
         (UINT64_C(1) << 63) | (UINT64_C(0) << 52) | UINT64_C(54651844)},
    };
    static const struct
    {
        float fl;
        uint32_t u32;
    } FLOAT_VALUES[] = {
        {0.0f, (UINT32_C(0) << 31) | (UINT32_C(0) << 23) | UINT32_C(0)},
        {0.0000000000000000000000000000000000000037157586781207584853831912444f,
         (UINT32_C(0) << 31) | (UINT32_C(0) << 23) | UINT32_C(2651654)},
        {-0.0000000000000000000000000000000000000037157586781207584853831912444f,
         (UINT32_C(1) << 31) | (UINT32_C(0) << 23) | UINT32_C(2651654)},
    };

    for (const auto &i : DOUBLE_VALUES)
    {
        EXPECT_EQ(i.u64, encode_double_to_u64(i.d));
        EXPECT_EQ(i.d, decode_u64_to_double(i.u64));
    }

    for (const auto &i : FLOAT_VALUES)
    {
        EXPECT_EQ(i.u32, encode_float_to_u32(i.fl));
        EXPECT_EQ(i.fl, decode_u32_to_float(i.u32));
    }
}

}  // namespace
