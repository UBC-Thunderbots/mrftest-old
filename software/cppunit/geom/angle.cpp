#include "geom/angle.h"
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

namespace
{
class AngleTest final : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(AngleTest);
    CPPUNIT_TEST(test_statics);
    CPPUNIT_TEST(test_of_radians);
    CPPUNIT_TEST(test_of_degrees);
    CPPUNIT_TEST(test_angle_mod);
    CPPUNIT_TEST(test_angle_diff);
    CPPUNIT_TEST_SUITE_END();

   public:
    void test_statics();
    void test_of_radians();
    void test_of_degrees();
    void test_angle_mod();
    void test_angle_diff();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AngleTest);
}

void AngleTest::test_statics()
{
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0, Angle::zero().to_degrees(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(90, Angle::quarter().to_degrees(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(180, Angle::half().to_degrees(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(270, Angle::three_quarter().to_degrees(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(360, Angle::full().to_degrees(), 0.1);
}

void AngleTest::test_of_radians()
{
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0, Angle::of_radians(0).to_degrees(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        30, Angle::of_radians(M_PI / 6).to_degrees(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        390, Angle::of_radians(M_PI * 13 / 6).to_degrees(), 0.1);
}

void AngleTest::test_of_degrees()
{
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0, Angle::of_degrees(0).to_radians(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        M_PI / 3, Angle::of_degrees(60).to_radians(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        -M_PI, Angle::of_degrees(-180).to_radians(), 0.01);
}

void AngleTest::test_angle_mod()
{
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        27, Angle::of_degrees(27).angle_mod().to_degrees(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        27, Angle::of_degrees(360 + 27).angle_mod().to_degrees(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        -27, Angle::of_degrees(360 - 27).angle_mod().to_degrees(), 0.1);
}

void AngleTest::test_angle_diff()
{
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        27,
        Angle::of_degrees(50).angle_diff(Angle::of_degrees(23)).to_degrees(),
        0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        27,
        Angle::of_degrees(360 + 13)
            .angle_diff(Angle::of_degrees(-14))
            .to_degrees(),
        0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        27,
        Angle::of_degrees(180 - 13)
            .angle_diff(Angle::of_degrees(-180 + 14))
            .to_degrees(),
        0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        27,
        Angle::of_degrees(180 + 13)
            .angle_diff(Angle::of_degrees(-180 - 14))
            .to_degrees(),
        0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        27,
        Angle::of_degrees(-180 + 13)
            .angle_diff(Angle::of_degrees(180 - 14))
            .to_degrees(),
        0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        27,
        Angle::of_degrees(-180 - 13)
            .angle_diff(Angle::of_degrees(180 + 14))
            .to_degrees(),
        0.1);
}
