#include "geom/angle.h"
#include <gtest/gtest.h>

namespace
{
TEST(AngleTest, Statics)
{
    EXPECT_DOUBLE_EQ(0, Angle::zero().to_degrees());
    EXPECT_DOUBLE_EQ(90, Angle::quarter().to_degrees());
    EXPECT_DOUBLE_EQ(180, Angle::half().to_degrees());
    EXPECT_DOUBLE_EQ(270, Angle::three_quarter().to_degrees());
    EXPECT_DOUBLE_EQ(360, Angle::full().to_degrees());
}

TEST(AngleTest, Of_radians)
{
    EXPECT_DOUBLE_EQ(0, Angle::of_radians(0).to_degrees());
    EXPECT_DOUBLE_EQ(30, Angle::of_radians(M_PI / 6).to_degrees());
    EXPECT_DOUBLE_EQ(390, Angle::of_radians(M_PI * 13 / 6).to_degrees());
}

TEST(AngleTest, Of_degrees)
{
    EXPECT_DOUBLE_EQ(0, Angle::of_degrees(0).to_radians());
    EXPECT_DOUBLE_EQ(M_PI / 3, Angle::of_degrees(60).to_radians());
    EXPECT_DOUBLE_EQ(M_PI, Angle::of_degrees(180).to_radians());
    EXPECT_DOUBLE_EQ(-M_PI, Angle::of_degrees(-180).to_radians());
}

TEST(AngleTest, Angle_mod)
{
    EXPECT_DOUBLE_EQ(27, Angle::of_degrees(27).angle_mod().to_degrees());
    EXPECT_DOUBLE_EQ(27, Angle::of_degrees(360 + 27).angle_mod().to_degrees());
    EXPECT_DOUBLE_EQ(-27, Angle::of_degrees(360 - 27).angle_mod().to_degrees());
}

TEST(AngleTest, Angle_diff)
{
    EXPECT_DOUBLE_EQ(
        27,
        Angle::of_degrees(50).angle_diff(Angle::of_degrees(23)).to_degrees());
    // We require a slightly larger tolerance for this test to pass
    EXPECT_NEAR(
        27,
        Angle::of_degrees(360 + 13)
            .angle_diff(Angle::of_degrees(-14))
            .to_degrees(),
        1e-13);
    EXPECT_DOUBLE_EQ(
        27,
        Angle::of_degrees(180 - 13)
            .angle_diff(Angle::of_degrees(-180 + 14))
            .to_degrees());
    EXPECT_DOUBLE_EQ(
        27,
        Angle::of_degrees(180 + 13)
            .angle_diff(Angle::of_degrees(-180 - 14))
            .to_degrees());
    EXPECT_DOUBLE_EQ(
        27,
        Angle::of_degrees(-180 + 13)
            .angle_diff(Angle::of_degrees(180 - 14))
            .to_degrees());
    EXPECT_DOUBLE_EQ(
        27,
        Angle::of_degrees(-180 - 13)
            .angle_diff(Angle::of_degrees(180 + 14))
            .to_degrees());
}

}  // namespace
