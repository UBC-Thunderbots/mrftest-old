#include "util/codec.h"
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cmath>

namespace
{
class CodecTest final : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CodecTest);
    CPPUNIT_TEST(test_zeroes);
    CPPUNIT_TEST(test_nans);
    CPPUNIT_TEST(test_infinities);
    CPPUNIT_TEST(test_normals);
    CPPUNIT_TEST(test_subnormals);
    CPPUNIT_TEST_SUITE_END();

   public:
    void test_zeroes();
    void test_nans();
    void test_infinities();
    void test_normals();
    void test_subnormals();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CodecTest);
}

void CodecTest::test_zeroes()
{
    CPPUNIT_ASSERT_EQUAL(0.0, decode_u64_to_double(0));
    CPPUNIT_ASSERT_EQUAL(
        -0.0, decode_u64_to_double(UINT64_C(0x8000000000000000)));
    CPPUNIT_ASSERT_EQUAL(UINT64_C(0), encode_double_to_u64(0.0));
    CPPUNIT_ASSERT_EQUAL(
        UINT64_C(0x8000000000000000), encode_double_to_u64(-0.0));
    CPPUNIT_ASSERT_EQUAL(0.0f, decode_u32_to_float(0));
    CPPUNIT_ASSERT_EQUAL(-0.0f, decode_u32_to_float(UINT32_C(0x80000000)));
    CPPUNIT_ASSERT_EQUAL(UINT32_C(0), encode_float_to_u32(0.0f));
    CPPUNIT_ASSERT_EQUAL(UINT32_C(0x80000000), encode_float_to_u32(-0.0f));
}

void CodecTest::test_nans()
{
    CPPUNIT_ASSERT(
        std::isnan(decode_u64_to_double(UINT64_C(0x7FF0000000000001))));
    CPPUNIT_ASSERT_EQUAL(
        UINT64_C(0x7FF0000000000000),
        encode_double_to_u64(NAN) & UINT64_C(0x7FF0000000000000));
    CPPUNIT_ASSERT(
        (encode_double_to_u64(NAN) & UINT64_C(0x000FFFFFFFFFFFFF)) != 0);
    CPPUNIT_ASSERT(std::isnan(decode_u32_to_float(UINT32_C(0x7F800001))));
    CPPUNIT_ASSERT_EQUAL(
        UINT32_C(0x7F800000), encode_float_to_u32(NAN) & UINT32_C(0x7F800000));
    CPPUNIT_ASSERT((encode_float_to_u32(NAN) & UINT32_C(0x007FFFFF)) != 0);
}

void CodecTest::test_infinities()
{
    CPPUNIT_ASSERT(
        std::isinf(decode_u64_to_double(UINT64_C(0x7FF0000000000000))));
    CPPUNIT_ASSERT(decode_u64_to_double(UINT64_C(0x7FF0000000000000)) > 0);
    CPPUNIT_ASSERT_EQUAL(
        UINT64_C(0x7FF0000000000000), encode_double_to_u64(INFINITY));
    CPPUNIT_ASSERT(
        std::isinf(decode_u64_to_double(UINT64_C(0xFFF0000000000000))));
    CPPUNIT_ASSERT(decode_u64_to_double(UINT64_C(0xFFF0000000000000)) < 0);
    CPPUNIT_ASSERT_EQUAL(
        UINT64_C(0xFFF0000000000000), encode_double_to_u64(-INFINITY));
    CPPUNIT_ASSERT(std::isinf(decode_u32_to_float(UINT32_C(0x7F800000))));
    CPPUNIT_ASSERT(decode_u32_to_float(UINT32_C(0x7F800000)) > 0);
    CPPUNIT_ASSERT_EQUAL(UINT32_C(0x7F800000), encode_float_to_u32(INFINITY));
    CPPUNIT_ASSERT(std::isinf(decode_u32_to_float(UINT32_C(0xFF800000))));
    CPPUNIT_ASSERT(decode_u32_to_float(UINT32_C(0xFF800000)) < 0);
    CPPUNIT_ASSERT_EQUAL(UINT32_C(0xFF800000), encode_float_to_u32(-INFINITY));
}

void CodecTest::test_normals()
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
        CPPUNIT_ASSERT_EQUAL(i.u64, encode_double_to_u64(i.d));
        CPPUNIT_ASSERT_EQUAL(i.d, decode_u64_to_double(i.u64));
    }

    for (const auto &i : FLOAT_VALUES)
    {
        CPPUNIT_ASSERT_EQUAL(i.u32, encode_float_to_u32(i.fl));
        CPPUNIT_ASSERT_EQUAL(i.fl, decode_u32_to_float(i.u32));
    }
}

void CodecTest::test_subnormals()
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
        CPPUNIT_ASSERT_EQUAL(i.u64, encode_double_to_u64(i.d));
        CPPUNIT_ASSERT_EQUAL(i.d, decode_u64_to_double(i.u64));
    }

    for (const auto &i : FLOAT_VALUES)
    {
        CPPUNIT_ASSERT_EQUAL(i.u32, encode_float_to_u32(i.fl));
        CPPUNIT_ASSERT_EQUAL(i.fl, decode_u32_to_float(i.u32));
    }
}
