#include "geom/util.h"
#include <gtest/gtest.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include "geom/angle.h"
#include "geom/point.h"

// Set this to 1 to enable debug output.
#define DEBUG 0
using namespace Geom;

namespace
{
#if DEBUG
#define dbgout std::cout
#else
std::ostringstream dbgout;
#endif

TEST(GeomUtilTest, dist_line_vector2)
{
    double calculated_val, expected_val;

    // case 1
    Line test1line(Vector2(0, 1), Vector2(0, 0));
    Vector2 test1p(2, 0);
    calculated_val = dist(test1line, test1p);
    expected_val   = 2;
    EXPECT_EQ(expected_val, calculated_val);

    // case 2
    Line test2line(Vector2(2, 0), Vector2(0, 2));
    Vector2 test2p(0, 0);
    calculated_val = dist(test2line, test2p);
    expected_val   = sqrt(2);
    EXPECT_DOUBLE_EQ(expected_val, calculated_val);

    // case 3
    Line test3line(Vector2(0, 0), Vector2(0, 0));
    Point test3p(1, 0);
    calculated_val = dist(test3line, test3p);
    expected_val   = 1;
    EXPECT_EQ(expected_val, calculated_val);

    Line line(Vector2(1, -1), Vector2(5, -2));
    Point p(2, -3);

    EXPECT_NEAR(1.69775, dist(line, p), 1e-5);

    p = Point(2, 1);
    EXPECT_NEAR(2.18282, dist(line, p), 1e-5);

    p = Point(2, 0);
    EXPECT_NEAR(1.21268, dist(line, p), 1e-5);
}

TEST(GeomUtilTest, test_proj_len)
{
    double calculated_val, expected_val;
    // test case 1
    Point test1p1(0, 0);
    Point test1p2(4, 4);
    Point test1p3(4, 0);
    calculated_val = proj_len(Seg(test1p1, test1p2), test1p3);
    expected_val   = 2 * sqrt(2);
    EXPECT_DOUBLE_EQ(expected_val, calculated_val);

    // test case 2
    Point test2p1(0, 0);
    Point test2p2(4, 0);
    Point test2p3(4, 4);
    calculated_val = proj_len(Seg(test2p1, test2p2), test2p3);
    expected_val   = 4;
    EXPECT_EQ(expected_val, calculated_val);

    // test case 3
    Point test3p1(0, 0);
    Point test3p2(4, 4);
    Point test3p3(-4, -4);
    calculated_val = proj_len(Seg(test3p1, test3p2), test3p3);
    expected_val   = -4 * sqrt(2);
    EXPECT_DOUBLE_EQ(expected_val, calculated_val);

    // test case 4
    Point test4p1(0, 0);
    Point test4p2(4, 1);
    Point test4p3(-4, -4);
    calculated_val = proj_len(Seg(test4p1, test4p2), test4p3);
    expected_val   = -sqrt(32) * (cos((M_PI / 4.0f) - atan(1.0f / 4.0f)));
    EXPECT_DOUBLE_EQ(expected_val, calculated_val);
}

TEST(GeomUtilTest, test_contains_triangle_point)
{
    // this triangle lies in the first quatren of the field, we can rota
    Point p1(0, 0);
    Point p2((std::rand() % 100) / 100, 0);
    Point p3((std::rand() % 100) / 100, (std::rand() % 100) / 100);
    Point p((std::rand() % 100) / 100, (std::rand() % 100) / 100);
    bool expected_val = true;

    /* i don't know what is going on here, this part seems to not work very well
    if (p.y <= p3.y && p.x >= p.y / p3.y * p3.x && p.x <= (p.y / p3.y * (p3.x -
    p2.x) + p2.x)) {
            expected_val = true;            // so the point is inside the
    triangle,
    }
    Angle rot = Angle::of_degrees(std::rand());
    Point trans(1.2, 1.2);
    p1 = p1.rotate(rot);
    p2 = p2.rotate(rot);
    p3 = p3.rotate(rot);
    p = p.rotate(rot);
    p1 += trans;
    p2 += trans;
    p3 += trans;
    p += trans;
    */

    // so we'll just abuse cross products to see on which side of each side of
    // the triangle it's on
    if (((p2 - p1).cross(p - p1) > 0) != ((p2 - p1).cross(p3 - p1) > 0))
        expected_val = false;
    if (((p3 - p2).cross(p - p2) > 0) != ((p3 - p2).cross(p1 - p2) > 0))
        expected_val = false;
    if (((p1 - p3).cross(p - p3) > 0) != ((p1 - p3).cross(p2 - p3) > 0))
        expected_val = false;

    bool calculated_val = contains(triangle(p1, p2, p3), p);
    EXPECT_EQ(expected_val, calculated_val);
}

TEST(GeomUtilTest, test_collinear)
{
    for (unsigned int i = 0; i < 10; ++i)
    {
        Point v = Point::of_angle(Angle::of_degrees(
            std::rand() % 360));  // should be random number here
        Point pointA((std::rand() % 100) / 100.0, (std::rand() % 100) / 100.0);
        Point pointB = pointA + v * (std::rand() % 100) / 100.0;
        Point pointC = pointA - v * (std::rand() % 100) / 100.0;
        bool val     = collinear(pointA, pointB, pointC);
        EXPECT_TRUE(val);
    }
}

TEST(GeomUtilTest, test_intersects_triangle_circle)
{
    Point test1p1(-5, 0);
    Point test1p2(5, 0);
    Point test1p3(2, 5);
    Point test1c(0, -1);
    double test1radius = 1;
    EXPECT_TRUE(!intersects(
        triangle(test1p1, test1p2, test1p3),
        Circle(
            test1c,
            test1radius)));  // circle is tangent to triangle, no intersect

    Point test2p1(-10, 0);
    Point test2p2(10, 0);
    Point test2p3(0, 15);
    Point test2c(0, 5);
    double test2radius = 1;
    EXPECT_TRUE(intersects(
        triangle(test2p1, test2p2, test2p3),
        Circle(
            test2c,
            test2radius)));  // circle is completely inside triangle, intersect

    Point test3p1(-5, -5);
    Point test3p2(5, -5);
    Point test3p3(0, 0);
    Point test3c(0, 1);
    double test3radius = 1;
    EXPECT_TRUE(!intersects(
        triangle(test3p1, test3p2, test3p3),
        Circle(
            test3c,
            test3radius)));  // circle is tangent to vertice, no intersect

    Point test4p1(-8, -5);
    Point test4p2(0, 0);
    Point test4p3(-3, -2);
    Point test4c(5, 5);
    double test4radius = 2;
    EXPECT_TRUE(!intersects(
        triangle(test4p1, test4p2, test4p3), Circle(test4c, test4radius)));

    Point test5p1(-2, -2);
    Point test5p2(2, -2);
    Point test5p3(0, 1);
    Point test5c(0, -1);
    double test5radius = 1;
    EXPECT_TRUE(intersects(
        triangle(test5p1, test5p2, test5p3), Circle(test5c, test5radius)));
}

TEST(GeomUtilTest, test_dist_matching)
{
    std::vector<Point> v1;
    std::vector<Point> v2;

    v1.push_back(Point(1, 4));
    v1.push_back(Point(1, 2));
    v1.push_back(Point(2, -3));
    v1.push_back(Point(2, -4));
    v2.push_back(Point(-1, 5));
    v2.push_back(Point(-1, 2.5));
    v2.push_back(Point(-1, -3));
    v2.push_back(Point(-1, -6));

    std::vector<std::size_t> match = dist_matching(v1, v2);

    EXPECT_TRUE(match[0] == 0);
    EXPECT_TRUE(match[1] == 1);
    EXPECT_TRUE(match[2] == 2);
    EXPECT_TRUE(match[3] == 3);

    match.clear();
    v1.clear();
    v2.clear();

    v1.push_back(Point(1, 4));
    v1.push_back(Point(2, -3));
    v1.push_back(Point(1, 2));
    v1.push_back(Point(2, -4));
    v2.push_back(Point(-1, 5));
    v2.push_back(Point(-1, 2.5));
    v2.push_back(Point(-1, -3));
    v2.push_back(Point(-1, -6));

    match = dist_matching(v1, v2);

    EXPECT_TRUE(match[0] == 0);
    EXPECT_TRUE(match[1] == 2);
    EXPECT_TRUE(match[2] == 1);
    EXPECT_TRUE(match[3] == 3);
}

TEST(GeomUtilTest, test_angle_sweep_circles)
{
    std::vector<Point> obs;
    obs.clear();
    obs.push_back(Point(-9, 10));
    obs.push_back(Point(9, 10));

    std::pair<Point, Angle> testpair = angle_sweep_circles(
        Point(0, 0), Point(10, 10), Point(-10, 10), obs, 1.0);

    EXPECT_TRUE((testpair.first.norm() - Point(0, 1)).len() < 0.0001);
    EXPECT_NEAR(75.449, testpair.second.to_degrees(), 1e-4);

    obs.clear();
    obs.push_back(Point(-4, 6));
    obs.push_back(Point(6, 8));
    obs.push_back(Point(4, 10));

    testpair = angle_sweep_circles(
        Point(0, 0), Point(10, 10), Point(-10, 10), obs, 1.0);

    EXPECT_TRUE(
        (testpair.first.norm() - Point(-0.0805897, 0.996747)).len() < 0.0001);
    EXPECT_NEAR(42.1928, testpair.second.to_degrees(), 1e-4);
}

TEST(GeomUtilTest, test_angle_sweep_circles_all)
{
    std::vector<Point> obs;
    obs.clear();
    obs.push_back(Point(-9, 10));
    obs.push_back(Point(9, 10));

    std::vector<std::pair<Point, Angle>> testpairs = angle_sweep_circles_all(
        Point(0, 0), Point(10, 10), Point(-10, 10), obs, 1.0);

    obs.clear();
    obs.push_back(Point(-4, 6));
    obs.push_back(Point(6, 8));
    obs.push_back(Point(4, 10));

    testpairs = angle_sweep_circles_all(
        Point(0, 0), Point(10, 10), Point(-10, 10), obs, 1.0);

    // TODO: Add assert statement
}

TEST(GeomUtilTest, test_point_in_rectangle)
{
    // Point in 1st quadrant, rectangle in the 3rd quadrant. Should fail!
    EXPECT_TRUE(!contains(Rect(Point(0, 0), Point(-2, -2)), Point(1, 1)));

    // Point in 3rd quadrant, rectangle in the 3rd quadrant. Pass!
    EXPECT_TRUE(contains(Rect(Point(0, 0), Point(-2, -2)), Point(-1, -1)));

    // Point is one of the corners of the rectangle. Pass
    EXPECT_TRUE(contains(Rect(Point(0, 0), Point(2, 2)), Point(2, 2)));

    // Point is on the edge of the rectangle. Pass
    EXPECT_TRUE(contains(Rect(Point(0, 0), Point(3, 3)), Point(0, 1)));

    // Point in the 1st quadrant, rectangle in the 1st quadrant. Pass
    EXPECT_TRUE(contains(Rect(Point(0, 0), Point(3, 3)), Point(1, 2)));

    // Point in the 2nd quadrant, rectangle in the 2nd quadrant. Point is off
    // above, Fail.
    EXPECT_TRUE(!contains(Rect(Point(0, 0), Point(-4, 4)), Point(-2, 5)));

    // Point in the 2nd quadrant, rectangle in the 4th quadrant. Point is off to
    // the left, Fail.
    EXPECT_TRUE(!contains(Rect(Point(0, 0), Point(-4, 4)), Point(-7, 2)));

    // Point in the 2nd quadrant, rectangle centered at origin. Point is off
    // above, Fail.
    EXPECT_TRUE(contains(Rect(Point(1, 1), Point(-1, -1)), Point(0.5, 0.5)));

    // Point in the 2nd quadrant, rectangle centered at origin. Point is off to
    // the left, Fail.
    EXPECT_TRUE(!contains(Rect(Point(1, 1), Point(-1, -1)), Point(2, 2)));
}

TEST(GeomUtilTest, test_seg_buffer_boundaries)
{
    Point a = Point(0, 0);
    Point b = Point(1, 0);
    for (Point i : seg_buffer_boundaries(a, b, 1.0, 10))
        EXPECT_DOUBLE_EQ(1.0, dist(i, Seg(a, b)));

    a = Point(5, 2);
    b = Point(2, 7);

    for (Point i : seg_buffer_boundaries(a, b, 4.0, 10))
        EXPECT_DOUBLE_EQ(4.0, dist(i, Seg(a, b)));
}

TEST(GeomUtilTest, test_circle_boundaries)
{
    std::vector<Point> test_circle = circle_boundaries(Point(0, 0), 1, 6);

    for (Point i : test_circle)
        EXPECT_DOUBLE_EQ(1.0, (i - Point(0, 0)).len());
}

TEST(GeomUtilTest, test_closest_lineseg_point)
{
    Point l1(-1, 1);
    Point l2(1, 1);

    EXPECT_TRUE(
        (closest_lineseg_point(Point(0, 2), l1, l2) - Point(0, 1)).len() <
        0.00001);
    EXPECT_TRUE(
        (closest_lineseg_point(Point(-2, 1.5), l1, l2) - Point(-1, 1)).len() <
        0.00001);

    l1 = Point(-2, 1);
    l2 = Point(1, 2);

    EXPECT_TRUE(
        (closest_lineseg_point(Point(1, 0), l1, l2) - Point(0.4, 1.8)).len() <
        0.00001);
    EXPECT_TRUE(
        (closest_lineseg_point(Point(-1.4, 1.2), l1, l2) - Point(-1.4, 1.2))
            .len() < 0.00001);
}

TEST(GeomUtilTest, test_line_circle_intersect)
{
    std::vector<Point> intersections =
        line_circle_intersect(Point(0, 0), 1.0, Point(0, 3), Point(1, 3));
    EXPECT_TRUE(intersections.size() == 0);

    intersections =
        line_circle_intersect(Point(0, 0), 1.0, Point(-1, 1), Point(1, 1));
    EXPECT_TRUE(intersections.size() == 1);
    EXPECT_TRUE((intersections[0] - Point(0, 1)).len() < 0.00001);

    // i don't know which intersections will come in which order
    intersections =
        line_circle_intersect(Point(0, 0), 1.0, Point(-1, -1), Point(1, 1));
    EXPECT_TRUE(intersections.size() == 2);
    EXPECT_TRUE(
        (intersections[0] - Point(1.0 / sqrt(2.0), 1.0 / sqrt(2.0))).len() <
            0.00001 ||
        (intersections[0] - Point(-1.0 / sqrt(2.0), -1.0 / sqrt(2.0))).len() <
            0.00001);
    EXPECT_TRUE(
        (intersections[1] - Point(1.0 / sqrt(2.0), 1.0 / sqrt(2.0))).len() <
            0.00001 ||
        (intersections[1] - Point(-1.0 / sqrt(2.0), -1.0 / sqrt(2.0))).len() <
            0.00001);
}

TEST(GeomUtilTest, test_line_rect_intersect)
{
    std::vector<Point> intersections = line_rect_intersect(
        Rect(Point(-1, -1), Point(1, 1)), Point(-1, -2), Point(1, 2));

    EXPECT_TRUE(intersections.size() == 2);
    EXPECT_TRUE(
        (intersections[0] - Point(0.5, 1)).len() < 0.00001 ||
        (intersections[0] - Point(-0.5, -1)).len() < 0.00001);
    EXPECT_TRUE(
        (intersections[1] - Point(0.5, 1)).len() < 0.00001 ||
        (intersections[1] - Point(-0.5, -1)).len() < 0.00001);

    intersections = line_rect_intersect(
        Rect(Point(0, 0), Point(1, 2)), Point(-1, 0), Point(4, 2));

    EXPECT_TRUE(intersections.size() == 2);
    EXPECT_TRUE(
        (intersections[0] - Point(0, 0.4)).len() < 0.00001 ||
        (intersections[0] - Point(1, 0.8)).len() < 0.00001);
    EXPECT_TRUE(
        (intersections[1] - Point(0, 0.4)).len() < 0.00001 ||
        (intersections[1] - Point(1, 0.8)).len() < 0.00001);

    intersections = line_rect_intersect(
        Rect(Point(-1, -1), Point(1, 1)), Point(0, 0), Point(1, 2));

    EXPECT_TRUE(intersections.size() == 1);
    EXPECT_TRUE((intersections[0] - Point(0.5, 1)).len() < 0.00001);

    intersections = line_rect_intersect(
        Rect(Point(-1, -1), Point(1, 1)), Point(-0.5, -0.5), Point(0.5, 0.5));

    EXPECT_TRUE(intersections.size() == 0);
}

TEST(GeomUtilTest, test_vector_rect_intersect)
{
    dbgout << "========= Enter vector_rect_intersect Test ========="
           << std::endl;
    Rect rect({1.0, 1.0}, {-1.0, -1.0});
    Point pr1(((std::rand() % 200) - 100) / 100.0, 1.0);
    Point pr2(((std::rand() % 200) - 100) / 100.0, -1.0);
    Point pr3(1.0, ((std::rand() % 200) - 100) / 100.0);
    Point pr4(-1.0, ((std::rand() % 200) - 100) / 100.0);
    Point pb(
        ((std::rand() % 200) - 100) / 100.0,
        ((std::rand() % 200) - 100) / 100.0);
    Point pe1    = (pr1 - pb).norm() + pr1;
    Point pe2    = (pr2 - pb).norm() + pr2;
    Point pe3    = (pr3 - pb).norm() + pr3;
    Point pe4    = (pr4 - pb).norm() + pr4;
    Point found1 = vector_rect_intersect(rect, pb, pr1);
    Point found2 = vector_rect_intersect(rect, pb, pr2);
    Point found3 = vector_rect_intersect(rect, pb, pr3);
    Point found4 = vector_rect_intersect(rect, pb, pr4);

    // uncomment to print out some debugging info
    dbgout << " vectorA (" << pb.x << ", " << pb.y << ") " << std::endl;
    dbgout << " Intersect1 (" << pr1.x << ", " << pr1.y << ") "
           << " found1 (" << found1.x << ", " << found1.y << ") " << std::endl;
    dbgout << " Intersect2 (" << pr2.x << ", " << pr2.y << ") "
           << " found2 (" << found2.x << ", " << found2.y << ") " << std::endl;
    dbgout << " Intersect3 (" << pr3.x << ", " << pr3.y << ") "
           << " found3 (" << found3.x << ", " << found3.y << ") " << std::endl;
    dbgout << " Intersect4 (" << pr4.x << ", " << pr4.y << ") "
           << " found4 (" << found4.x << ", " << found4.y << ") " << std::endl;
    dbgout << " vectorB1 (" << pe1.x << ", " << pe1.y << ") " << std::endl;
    dbgout << " vectorB2 (" << pe2.x << ", " << pe2.y << ") " << std::endl;
    dbgout << " vectorB3 (" << pe3.x << ", " << pe3.y << ") " << std::endl;
    dbgout << " vectorB4 (" << pe4.x << ", " << pe4.y << ") " << std::endl;

    EXPECT_TRUE((found1 - pr1).len() < 0.001);
    EXPECT_TRUE((found2 - pr2).len() < 0.001);
    EXPECT_TRUE((found3 - pr3).len() < 0.001);
    EXPECT_TRUE((found4 - pr4).len() < 0.001);
}

TEST(GeomUtilTest, test_clip_point)
{
    Point rect1(-2, -1);
    Point rect2(2, 1);

    EXPECT_TRUE(
        (clip_point(Point(1, 1), rect1, rect2) - Point(1, 1)).len() < 0.00001);
    EXPECT_TRUE(
        (clip_point(Point(3, 1), rect1, rect2) - Point(2, 1)).len() < 0.00001);
    EXPECT_TRUE(
        (clip_point(Point(3, 2), rect1, rect2) - Point(2, 1)).len() < 0.00001);
}

TEST(GeomUtilTest, test_clip_point2)
{
    Rect r(Point(-2, -1), Point(2, 1));

    EXPECT_TRUE((clip_point(Point(1, 1), r) - Point(1, 1)).len() < 0.00001);
    EXPECT_TRUE((clip_point(Point(3, 1), r) - Point(2, 1)).len() < 0.00001);
    EXPECT_TRUE((clip_point(Point(3, 2), r) - Point(2, 1)).len() < 0.00001);
}

TEST(GeomUtilTest, test_unique_line_intersect)
{
    EXPECT_TRUE(unique_line_intersect(
        Point(0, 0), Point(2, 2), Point(1, 0), Point(0, 1)));
    EXPECT_TRUE(!unique_line_intersect(
        Point(0, 0), Point(1, 1), Point(-1, 0), Point(0, 1)));
}

TEST(GeomUtilTest, test_line_intersect)
{
    dbgout << "========= Enter line_intersect Test ========" << std::endl;

    // should check for the the rare cases

    for (int i = 0; i < 10; i++)
    {
        // generate three random points
        Point a1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
        Point b1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
        Point expected(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);

        // We do not know what the  tolorance of the function is, but we
        // probabaly should check if segments overlap completely

        Point a2 = a1 + (expected - a1) * (1 + std::rand() % 200 / 100.0);
        Point b2 = b1 + (expected - b1) * (1 + std::rand() % 200 / 100.0);

        Point found = line_intersect(a1, a2, b1, b2);

        // uncomment to print out some messages
        dbgout << "points are (" << a1.x << ", " << a1.y << ") ";
        dbgout << " (" << a2.x << ", " << a2.y << ") ";
        dbgout << " (" << b1.x << ", " << b1.y << ") ";
        dbgout << " (" << b2.x << ", " << b2.y << ") " << std::endl;
        dbgout << "expecting (" << expected.x << ", " << expected.y << ") "
               << std::endl;
        dbgout << "found (" << found.x << ", " << found.y << ") " << std::endl;

        EXPECT_TRUE((expected - found).len() < 0.0001);
    }
}

TEST(GeomUtilTest, test_seg_crosses_seg)
{
    dbgout << "========= Enter seg_crosses_seg Test ========" << std::endl;

    // should check for the the rare cases

    for (int i = 0; i < 10; i++)
    {
        // generate three random points
        Point a1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
        Point b1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
        Point i0(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);

        // We do not know what the  tolorance of the function is, but we
        // probabaly should check if segments overlap completely

        bool a_over = std::rand() % 2;
        bool b_over = std::rand() % 2;

        Point a2 = a1 +
                   (i0 - a1) * (1 +
                                std::rand() % 100 / 100.0 *
                                    (a_over ? 1 : -1));  // the last part
                                                         // generate a number
                                                         // either bigger or
                                                         // smaller than 1
        Point b2 =
            b1 +
            (i0 - b1) *
                (1 +
                 std::rand() % 100 / 100.0 *
                     (b_over ? 1 : -1));  // as a scaling factor for a2 and b2

        bool expected = a_over && b_over;
        bool found    = intersects(Seg(a1, a2), Seg(b1, b2));

        // uncomment to print out some messages
        dbgout << "points are (" << a1.x << ", " << a1.y << ") ";
        dbgout << " (" << a2.x << ", " << a2.y << ") ";
        dbgout << " (" << b1.x << ", " << b1.y << ") ";
        dbgout << " (" << b2.x << ", " << b2.y << ") ";
        dbgout << " (" << i0.x << ", " << i0.y << ") ";
        dbgout << " a_over " << (a_over ? "true" : "false") << " b_over "
               << (b_over ? "true" : "false") << std::endl;
        dbgout << "expecting " << (expected ? "true" : "false") << " found "
               << (found ? "true" : "false") << std::endl;

        EXPECT_EQ(expected, found);
    }
}

TEST(GeomUtilTest, test_vector_crosses_seg)
{
    dbgout << "========= Enter vector_crosses_seg Test ========" << std::endl;

    // should check for the the rare cases

    // case where vector faces segment but segment may/ may not be long enough
    for (int i = 0; i < 5; i++)
    {
        // generate three random points
        Point a1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
        Point b1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
        Point i0(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);

        // We do not know what the  tolorance of the function is, but we
        // probabaly should check if segments overlap completely

        bool expected = std::rand() % 2;

        Point a2 = a1 + (i0 - a1).norm();
        Point b2 = b1 +
                   (i0 - b1) *
                       (1 +
                        std::rand() % 100 / 100.0 *
                            (expected ? 1 : -1));  // as a scaling factor for b2

        bool found = intersects(Ray(a1, a2), Seg(b1, b2));

        // uncomment to print out some messages
        dbgout << "points are (" << a1.x << ", " << a1.y << ") ";
        dbgout << " (" << a2.x << ", " << a2.y << ") ";
        dbgout << " (" << b1.x << ", " << b1.y << ") ";
        dbgout << " (" << b2.x << ", " << b2.y << ") ";
        dbgout << " (" << i0.x << ", " << i0.y << ") ";
        dbgout << "expecting " << (expected ? "true" : "false") << " found "
               << (found ? "true" : "false") << std::endl;

        EXPECT_EQ(expected, found);
    }

    // case where vector does not face segment
    for (int i = 0; i < 5; i++)
    {
        // generate three random points
        Point a1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
        Point b1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
        Point i0(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);

        // We do not know what the  tolorance of the function is, but we
        // probabaly should check if segments overlap completely

        bool expected = false;

        Point a2 = a1 - (i0 - a1).norm();
        Point b2 =
            b1 + (i0 - b1) * (1 + std::rand() % 100 / 100.0);  // as a scaling
                                                               // factor for b2,
                                                               // make sure it
                                                               // is long enough

        bool found = intersects(Ray(a1, a2), Seg(b1, b2));

        // uncomment to print out some messages
        dbgout << "points are (" << a1.x << ", " << a1.y << ") ";
        dbgout << " (" << a2.x << ", " << a2.y << ") ";
        dbgout << " (" << b1.x << ", " << b1.y << ") ";
        dbgout << " (" << b2.x << ", " << b2.y << ") ";
        dbgout << " (" << i0.x << ", " << i0.y << ") ";
        dbgout << "expecting " << (expected ? "true" : "false") << " found "
               << (found ? "true" : "false") << std::endl;

        EXPECT_EQ(expected, found);
    }
}

TEST(GeomUtilTest, test_reflect)
{
    Point ray0(2, 4);
    Point normal(-1, 1);
    Point reflected = reflect(ray0, normal);

    EXPECT_TRUE((reflected - Point(4, 2)).len() < 0.0001);
}

TEST(GeomUtilTest, test_reflect2)
{
    Point ray0(2, 4);
    Point line0(0, 0);
    Point line1(1, 1);
    Point reflected = reflect(line0, line1, ray0);

    EXPECT_TRUE((reflected - Point(4, 2)).len() < 0.0001);
}

TEST(GeomUtilTest, test_calc_block_cone)
{
    Point a(5, 10);
    Point b(-5, 10);

    EXPECT_TRUE((calc_block_cone(a, b, 1) - Point(0, sqrt(5))).len() < 0.00001);

    a = Point(0, 8);
    b = Point(4, 4);

    EXPECT_TRUE(
        (calc_block_cone(a, b, 1) - Point(1, 1.0 + sqrt(2))).len() < 0.00001);

    a = Point(2, -4);
    b = Point(6, -2);

    EXPECT_TRUE(
        (calc_block_cone(a, b, 1) - Point(1.9741, -1.71212)).len() < 0.00001);
}

TEST(GeomUtilTest, test_calc_block_cone2)
{
    Point a(5, 10);
    Point b(-5, 10);
    Point o(0, 0);

    EXPECT_TRUE(
        (calc_block_cone(a, b, o, 1) - Point(0, sqrt(5))).len() < 0.00001);

    a = Point(6, 11);
    b = Point(-4, 11);
    o = Point(1, 1);

    EXPECT_TRUE(
        (calc_block_cone(a, b, o, 1) - Point(0, sqrt(5)) - o).len() < 0.00001);

    a = Point(-2, 6);
    b = Point(2, 2);
    o = Point(-2, -2);

    EXPECT_TRUE(
        (calc_block_cone(a, b, o, 1) - Point(1, 1.0 + sqrt(2)) - o).len() <
        0.0001);
}

TEST(GeomUtilTest, test_calc_block_other_ray)
{
    // I don't know what the function is supposed to return, so I just set the
    // test value to the return value of the function for now.
    Point p(-0.301176, -1.24471);
    Point a = calc_block_other_ray(Point(1, 0), Point(0.2, 1), Point(0.4, 0.1));

    EXPECT_TRUE((a - p).len() < 0.00001);
}

TEST(GeomUtilTest, test_calc_goalie_block_goal_post)
{
    Point a(-1, 0);
    Point b(1.5, -0.5);
    Point c(0, 1);
    Point g(0, 0.5);

    EXPECT_TRUE(goalie_block_goal_post(a, b, c, g));
}

TEST(GeomUtilTest, test_calc_block_cone_defender)
{
    Point p(-0.353553, -0.0606602);
    Point a = calc_block_cone_defender(
        Point(1, 0), Point(-1, 0), Point(0, 1), Point(0.25, 0.5), 0.5);
    EXPECT_TRUE((a - p).len() < 0.00001);
}

TEST(GeomUtilTest, test_offset_to_line)
{
    Point x0(1, -1);
    Point x1(5, -2);
    Point p(2, -3);

    EXPECT_NEAR(-1.69775, offset_to_line(x0, x1, p), 1e-5);

    p = Point(2, 1);

    EXPECT_NEAR(2.18282, offset_to_line(x0, x1, p), 1e-5);

    p = Point(2, 0);

    EXPECT_NEAR(1.21268, offset_to_line(x0, x1, p), 1e-5);
}

TEST(GeomUtilTest, test_offset_along_line)
{
    Point x0(1, -1);
    Point x1(5, -2);
    Point p(2, 1);

    EXPECT_NEAR(0.485071, offset_along_line(x0, x1, p), 1e-5);

    p = Point(3, 1);

    EXPECT_NEAR(1.45521, offset_along_line(x0, x1, p), 1e-5);

    p = Point(2, -2);

    EXPECT_NEAR(1.21268, offset_along_line(x0, x1, p), 1e-5);
}

TEST(GeomUtilTest, test_segment_near_line)
{
    Point seg0(0, 3);
    Point seg1(3, 2);
    Point line0(-1, 0);
    Point line1(3, 5);

    EXPECT_TRUE(
        (segment_near_line(seg0, seg1, line0, line1) - Point(1.105263, 2.63158))
            .len() < 0.0001);

    seg0 = Point(0, 3);
    seg1 = Point(0, 4);

    EXPECT_TRUE(
        (segment_near_line(seg0, seg1, line0, line1) - Point(0, 3)).len() <
        0.001);
}

TEST(GeomUtilTest, test_intersection)
{
    Point a1(-1, 0);
    Point a2(4, 1);
    Point b1(0, -1);
    Point b2(1, 4);

    EXPECT_TRUE(
        (intersection(a1, a2, b1, b2) - Point(0.25, 0.25)).len() < 0.0001);

    a2 = Point(4, 2);

    EXPECT_TRUE(
        (intersection(a1, a2, b1, b2) - Point(0.30435, 0.52174)).len() <
        0.0001);
}

TEST(GeomUtilTest, test_vertex_angle)
{
    Point a(6, 2);
    Point b(0, 0);
    Point c(1, 5);

    EXPECT_NEAR(-60.2551, vertex_angle(a, b, c).to_degrees(), 1e-4);

    a = Point(6, 1);

    EXPECT_NEAR(-69.2277, vertex_angle(a, b, c).to_degrees(), 1e-4);
}

TEST(GeomUtilTest, test_closest_point_time)
{
    Point x1(0, 0);
    Point v1(1, 1);
    Point x2(2, 0);
    Point v2(-1, 1);

    EXPECT_DOUBLE_EQ(1.0, closest_point_time(x1, v1, x2, v2));

    x1 = Point(0, 0);
    v1 = Point(0, 0);
    x2 = Point(-1, 1);
    v2 = Point(1, 0);

    EXPECT_DOUBLE_EQ(1.0, closest_point_time(x1, v1, x2, v2));

    x1 = Point(0, 0);
    v1 = Point(1, 1);
    x2 = Point(6, 0);
    v2 = Point(-2, 2);

    EXPECT_DOUBLE_EQ(1.8, closest_point_time(x1, v1, x2, v2));
}

TEST(GeomUtilTest, test_dist_point_seg)
{
    Point a1(0, 0);
    Point b1(1, 0);

    EXPECT_DOUBLE_EQ(1.0, dist(Point(0, 1), Seg(a1, b1)));
    EXPECT_DOUBLE_EQ(1.0, dist(Point(2, 0), Seg(a1, b1)));
    EXPECT_DOUBLE_EQ(1.0, dist(Point(1, -1), Seg(a1, b1)));
    EXPECT_DOUBLE_EQ(1.0, dist(Point(-1, 0), Seg(a1, b1)));

    Point a2(5, 2);
    Point b2(2, 7);
    Point c2(6.5369, 7.2131);

    EXPECT_NEAR(4.0, dist(c2, Seg(a2, b2)), 1e-5);
}

}  // namespace
