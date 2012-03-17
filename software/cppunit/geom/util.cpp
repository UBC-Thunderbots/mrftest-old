#include "geom/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

// Set this to 1 to enable debug output.
#define DEBUG 0

namespace {
#if DEBUG
#define dbgout std::cout
#else
	std::ostringstream dbgout;
#endif

	class GeomUtilTest : public CppUnit::TestFixture {
		CPPUNIT_TEST_SUITE(GeomUtilTest);
		CPPUNIT_TEST(test_collinear);
		CPPUNIT_TEST(test_line_intersect);
		CPPUNIT_TEST(test_seg_crosses_seg);
		CPPUNIT_TEST(test_vector_crosses_seg);
		CPPUNIT_TEST(test_vector_rect_intersect);
		CPPUNIT_TEST(test_seg_pt_dist);
		CPPUNIT_TEST(test_proj_dist);
		CPPUNIT_TEST_SUITE_END();

		public:
			void test_line_pt_dist();
			void test_seg_pt_dist();
			void test_proj_dist();
			void test_point_in_triangle();
			void test_collinear();
			void test_triangle_circle_intersect();
			void test_dist_matching();
			void test_angle_sweep_circles();
			void test_angle_sweep_circles_all();
			void test_line_seg_intersect_rectangle();
			void test_point_in_rectangle();
			void test_seg_buffer_boundaries();
			void test_circle_boundaries();
			void test_lineseg_point_dist();
			void test_closet_lineseg_point();
			void test_line_circle_intersect();
			void test_line_circle_intersect2();
			void test_line_rect_intersect();
			void test_vector_rect_intersect();
			void test_clip_point();
			void test_unique_line_intersect();
			void test_line_intersect();
			void test_line_point_dist();
			void test_seg_crosses_seg();
			void test_vector_crosses_seg();
			void test_reflect();
			void test_reflect2();
			void test_calc_block_cone();
			void test_calc_block_cone2();
			void test_calc_block_cone3();
			void test_calc_block_other_ray();
			void test_calc_goalie_block_goal_post();
			void test_calc_block_cone_defender();
			void test_offset_to_line();
			void test_offset_along_line();
			void test_segment_near_line();
			void test_intersection();
			void test_vertex_angle();
			void test_closet_point_time();
	};
	CPPUNIT_TEST_SUITE_REGISTRATION(GeomUtilTest);
}

void GeomUtilTest::test_line_pt_dist() {
	double calculated_val, expected_val;

	//case 1
	Point test1linep1(0,1);
	Point test1linep2(0,0);
	Point test1p1(2,0);
	calculated_val = line_pt_dist(test1linep1, test1linep2, test1p1);
	expected_val = 2;
	CPPUNIT_ASSERT_EQUAL(expected_val, calculated_val);

	//case 2
	Point test2linep1(2,0);
	Point test2linep2(0,2);
	Point test2p1(0,0);
	calculated_val = line_pt_dist(test2linep1, test2linep2, test2p1);
	expected_val = sqrt(2);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(expected_val, calculated_val, 0.00001);

	//case 3
	Point test3linep1(0,0);
	Point test3linep2(0,0);
	Point test3p1(1,0);
	calculated_val = line_pt_dist(test3linep1, test3linep2, test3p1);
	expected_val = 1;
	CPPUNIT_ASSERT_EQUAL(expected_val, calculated_val);
}

void GeomUtilTest::test_seg_pt_dist() {
	double calculated_val, expected_val;

	//case 1
	Point test1linep1(1,0);
	Point test1linep2(2,3);
	Point test1p1(2,0);
	calculated_val = line_pt_dist(test1linep1, test1linep2, test1p1);
	expected_val = sin(atan(3));
	CPPUNIT_ASSERT_DOUBLES_EQUAL(expected_val, calculated_val, 0.0000001);

	//case 2
	Point test2linep1(2,0);
	Point test2linep2(0,2);
	Point test2p1(0,0);
	calculated_val = line_pt_dist(test2linep1, test2linep2, test2p1);
	expected_val = sqrt(2);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(expected_val, calculated_val, 0.000001);

	//case 3
	Point test3linep1(0,0);
	Point test3linep2(0,0);
	Point test3p1(1,0);
	calculated_val = line_pt_dist(test3linep1, test3linep2, test3p1);
	expected_val = 1;
	CPPUNIT_ASSERT_EQUAL(expected_val, calculated_val);
}

void GeomUtilTest::test_proj_dist() {
	double calculated_val, expected_val;
	//test case 1
	Point test1p1(0,0);
	Point test1p2(4,4);
	Point test1p3(4,0);
	calculated_val = proj_dist(test1p1, test1p2, test1p3);
	expected_val = 2*sqrt(2);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(expected_val, calculated_val, 0.000001);

	//test case 2
	Point test2p1(0,0);
	Point test2p2(4,0);
	Point test2p3(4,4);
	calculated_val = proj_dist(test2p1, test2p2, test2p3);
	expected_val = 4;
	CPPUNIT_ASSERT_EQUAL(expected_val, calculated_val);

	//test case 3
	Point test3p1(0,0);
	Point test3p2(4,4);
	Point test3p3(-4,-4);
	calculated_val = proj_dist(test3p1, test3p2, test3p3);
	expected_val = -4*sqrt(2);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(expected_val, calculated_val, 0.000001);

	//test case 4
	Point test4p1(0,0);
	Point test4p2(4,1);
	Point test4p3(-4,-4);
	calculated_val = proj_dist(test4p1, test4p2, test4p3);
	expected_val = -sqrt(32)*(cos((M_PI/4.0f)-atan(1.0f/4.0f)));
	CPPUNIT_ASSERT_DOUBLES_EQUAL(expected_val, calculated_val, 0.000001);

}

void GeomUtilTest::test_point_in_triangle() {
	// this triangle lies in the first quatren of the field, we can rota
	Point p1(0, 0);
	Point p2((std::rand() % 100) / 100, 0);
	Point p3((std::rand() % 100) / 100, (std::rand() % 100) / 100);
	Point p((std::rand() % 100) / 100, (std::rand() % 100) / 100);
	bool expected_val = false;
	if (p.y <= p3.y && p.x >= p.y / p3.y * p3.x && p.x <= (p.y / p3.y * (p3.x - p2.x) + p2.x)) {
		expected_val = true;            // so the point is inside the triangle,
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
	bool calculated_val = point_in_triangle(p1, p2, p3, p);
	CPPUNIT_ASSERT_EQUAL(expected_val, calculated_val);
}

void GeomUtilTest::test_triangle_circle_intersect() {
	Point test1p1(-5, 0);
	Point test1p2(5, 0);
	Point test1p3(2, 5);
	Point test1c(0, -1);
	double test1radius = 1;
	CPPUNIT_ASSERT(triangle_circle_intersect(test1p1,test1p2,test1p3,test1c,test1radius));

	Point test2p1(-10, 0);
	Point test2p2(10, 0);
	Point test2p3(0, 15);
	Point test2c(0, 5);
	double test2radius = 1;
	CPPUNIT_ASSERT(!triangle_circle_intersect(test2p1,test2p2,test2p3,test2c,test2radius));

	Point test3p1(-5, -5);
	Point test3p2(5, -5);
	Point test3p3(0, 0);
	Point test3c(0, 1);
	double test3radius = 1;
	CPPUNIT_ASSERT(triangle_circle_intersect(test3p1,test3p2,test3p3,test3c,test3radius));

	Point test4p1(-8, -5);
	Point test4p2(0, 0);
	Point test4p3(-3, -2);
	Point test4c(5, 5);
	double test4radius = 2;
	CPPUNIT_ASSERT(!triangle_circle_intersect(test4p1,test4p2,test4p3,test4c,test4radius));

	Point test5p1(-2, -2);
	Point test5p2(2, -2);
	Point test5p3(0, 1);
	Point test5c(0, -1);
	double test5radius = 1;
	CPPUNIT_ASSERT(triangle_circle_intersect(test5p1,test5p2,test5p3,test5c,test5radius));

}
void GeomUtilTest::test_dist_matching() {
	// not used anywhere
}
void GeomUtilTest::test_collinear() {
	for (unsigned int i = 0; i < 10; ++i) {
		Point v = Point::of_angle(Angle::of_degrees(std::rand() % 360)); // should be random number here
		Point pointA((std::rand() % 100) / 100.0, (std::rand() % 100) / 100.0);
		Point pointB = pointA + v * (std::rand() % 100) / 100.0;
		Point pointC = pointA - v * (std::rand() % 100) / 100.0;
		bool val = collinear(pointA, pointB, pointC);
		CPPUNIT_ASSERT(val);
	}
}

void GeomUtilTest::test_angle_sweep_circles() {
	// yes, this is used
}

void GeomUtilTest::test_angle_sweep_circles_all() {
}

void GeomUtilTest::test_line_seg_intersect_rectangle() {
}

void GeomUtilTest::test_point_in_rectangle() {
}

void GeomUtilTest::test_seg_buffer_boundaries() {
}

void GeomUtilTest::test_circle_boundaries() {
}

void GeomUtilTest::test_lineseg_point_dist() {
}

void GeomUtilTest::test_closet_lineseg_point() {
}

void GeomUtilTest::test_line_circle_intersect() {
}

void GeomUtilTest::test_line_circle_intersect2() {
}

void GeomUtilTest::test_line_rect_intersect() {
}

void GeomUtilTest::test_vector_rect_intersect() {
	dbgout << "========= Enter vector_rect_intersect Test =========" << std::endl;
	Rect rect({ 1.0, 1.0 }, { -1.0, -1.0 });
	Point pr1(((std::rand() % 200) - 100) / 100.0, 1.0);
	Point pr2(((std::rand() % 200) - 100) / 100.0, -1.0);
	Point pr3(1.0, ((std::rand() % 200) - 100) / 100.0);
	Point pr4(-1.0, ((std::rand() % 200) - 100) / 100.0);
	Point pb(((std::rand() % 200) - 100) / 100.0, ((std::rand() % 200) - 100) / 100.0);
	Point pe1 = (pr1 - pb).norm() + pr1;
	Point pe2 = (pr2 - pb).norm() + pr2;
	Point pe3 = (pr3 - pb).norm() + pr3;
	Point pe4 = (pr4 - pb).norm() + pr4;
	Point found1 = vector_rect_intersect(rect, pb, pr1);
	Point found2 = vector_rect_intersect(rect, pb, pr2);
	Point found3 = vector_rect_intersect(rect, pb, pr3);
	Point found4 = vector_rect_intersect(rect, pb, pr4);

	// uncomment to print out some debugging info
	dbgout << " vectorA (" << pb.x << ", " << pb.y << ") " << std::endl;
	dbgout << " Intersect1 (" << pr1.x << ", " << pr1.y << ") " << " found1 (" << found1.x << ", " << found1.y << ") " << std::endl;
	dbgout << " Intersect2 (" << pr2.x << ", " << pr2.y << ") " << " found2 (" << found2.x << ", " << found2.y << ") " << std::endl;
	dbgout << " Intersect3 (" << pr3.x << ", " << pr3.y << ") " << " found3 (" << found3.x << ", " << found3.y << ") " << std::endl;
	dbgout << " Intersect4 (" << pr4.x << ", " << pr4.y << ") " << " found4 (" << found4.x << ", " << found4.y << ") " << std::endl;
	dbgout << " vectorB1 (" << pe1.x << ", " << pe1.y << ") " << std::endl;
	dbgout << " vectorB2 (" << pe2.x << ", " << pe2.y << ") " << std::endl;
	dbgout << " vectorB3 (" << pe3.x << ", " << pe3.y << ") " << std::endl;
	dbgout << " vectorB4 (" << pe4.x << ", " << pe4.y << ") " << std::endl;

	CPPUNIT_ASSERT((found1 - pr1).len() < 0.001);
	CPPUNIT_ASSERT((found2 - pr2).len() < 0.001);
	CPPUNIT_ASSERT((found3 - pr3).len() < 0.001);
	CPPUNIT_ASSERT((found4 - pr4).len() < 0.001);
}

void GeomUtilTest::test_clip_point() {
}

void GeomUtilTest::test_unique_line_intersect() {
}

void GeomUtilTest::test_line_intersect() {
	dbgout << "========= Enter line_intersect Test ========" << std::endl;

// should check for the the rare cases

	for (int i = 0; i < 10; i++) {
		// generate three random points
		Point a1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
		Point b1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
		Point expected(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);

// We do not know what the  tolorance of the function is, but we probabaly should check if segments overlap completely

		Point a2 = a1 + (expected - a1) * (1 + std::rand() % 200 / 100.0);
		Point b2 = b1 + (expected - b1) * (1 + std::rand() % 200 / 100.0);

		Point found = line_intersect(a1, a2, b1, b2);

		// uncomment to print out some messages
		dbgout << "points are (" << a1.x << ", " << a1.y << ") ";
		dbgout << " (" << a2.x << ", " << a2.y << ") ";
		dbgout << " (" << b1.x << ", " << b1.y << ") ";
		dbgout << " (" << b2.x << ", " << b2.y << ") " << std::endl;
		dbgout << "expecting (" << expected.x << ", " << expected.y << ") " << std::endl;
		dbgout << "found (" << found.x << ", " << found.y << ") " << std::endl;

		CPPUNIT_ASSERT((expected - found).len() < 0.0001);
	}
}

void GeomUtilTest::test_line_point_dist() {
}

void GeomUtilTest::test_seg_crosses_seg() {
	dbgout << "========= Enter seg_crosses_seg Test ========" << std::endl;

// should check for the the rare cases

	for (int i = 0; i < 10; i++) {
		// generate three random points
		Point a1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
		Point b1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
		Point i0(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);

// We do not know what the  tolorance of the function is, but we probabaly should check if segments overlap completely

		bool a_over = std::rand() % 2;
		bool b_over = std::rand() % 2;

		Point a2 = a1 + (i0 - a1) * (1 + std::rand() % 100 / 100.0 * (a_over ? 1 : -1));    // the last part generate a number either bigger or smaller than 1
		Point b2 = b1 + (i0 - b1) * (1 + std::rand() % 100 / 100.0 * (b_over ? 1 : -1));    // as a scaling factor for a2 and b2

		bool expected = a_over && b_over;
		bool found = seg_crosses_seg(a1, a2, b1, b2);

		// uncomment to print out some messages
		dbgout << "points are (" << a1.x << ", " << a1.y << ") ";
		dbgout << " (" << a2.x << ", " << a2.y << ") ";
		dbgout << " (" << b1.x << ", " << b1.y << ") ";
		dbgout << " (" << b2.x << ", " << b2.y << ") ";
		dbgout << " (" << i0.x << ", " << i0.y << ") ";
		dbgout << " a_over " << (a_over ? "true" : "false") << " b_over " << (b_over ? "true" : "false") << std::endl;
		dbgout << "expecting " << (expected ? "true" : "false") << " found " << (found ? "true" : "false") << std::endl;

		CPPUNIT_ASSERT_EQUAL(expected, found);
	}
}

void GeomUtilTest::test_vector_crosses_seg() {
	dbgout << "========= Enter vector_crosses_seg Test ========" << std::endl;

// should check for the the rare cases

// case where vector faces segment but segment may/ may not be long enough
	for (int i = 0; i < 5; i++) {
		// generate three random points
		Point a1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
		Point b1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
		Point i0(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);

// We do not know what the  tolorance of the function is, but we probabaly should check if segments overlap completely

		bool expected = std::rand() % 2;

		Point a2 = a1 + (i0 - a1).norm();
		Point b2 = b1 + (i0 - b1) * (1 + std::rand() % 100 / 100.0 * (expected ? 1 : -1));  // as a scaling factor for b2

		bool found = vector_crosses_seg(a1, a2, b1, b2);

		// uncomment to print out some messages
		dbgout << "points are (" << a1.x << ", " << a1.y << ") ";
		dbgout << " (" << a2.x << ", " << a2.y << ") ";
		dbgout << " (" << b1.x << ", " << b1.y << ") ";
		dbgout << " (" << b2.x << ", " << b2.y << ") ";
		dbgout << " (" << i0.x << ", " << i0.y << ") ";
		dbgout << "expecting " << (expected ? "true" : "false") << " found " << (found ? "true" : "false") << std::endl;

		CPPUNIT_ASSERT_EQUAL(expected, found);
	}

// case where vector does not face segment
	for (int i = 0; i < 5; i++) {
		// generate three random points
		Point a1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
		Point b1(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);
		Point i0(std::rand() % 200 / 100.0, std::rand() % 200 / 100.0);

// We do not know what the  tolorance of the function is, but we probabaly should check if segments overlap completely

		bool expected = false;

		Point a2 = a1 - (i0 - a1).norm();
		Point b2 = b1 + (i0 - b1) * (1 + std::rand() % 100 / 100.0);   // as a scaling factor for b2, make sure it is long enough

		bool found = vector_crosses_seg(a1, a2, b1, b2);

		// uncomment to print out some messages
		dbgout << "points are (" << a1.x << ", " << a1.y << ") ";
		dbgout << " (" << a2.x << ", " << a2.y << ") ";
		dbgout << " (" << b1.x << ", " << b1.y << ") ";
		dbgout << " (" << b2.x << ", " << b2.y << ") ";
		dbgout << " (" << i0.x << ", " << i0.y << ") ";
		dbgout << "expecting " << (expected ? "true" : "false") << " found " << (found ? "true" : "false") << std::endl;

		CPPUNIT_ASSERT_EQUAL(expected, found);
	}
}

void GeomUtilTest::test_reflect() {
}

void GeomUtilTest::test_reflect2() {
}

void GeomUtilTest::test_calc_block_cone() {
}

void GeomUtilTest::test_calc_block_cone2() {
}

void GeomUtilTest::test_calc_block_cone3() {
}

void GeomUtilTest::test_calc_block_other_ray() {
}

void GeomUtilTest::test_calc_goalie_block_goal_post() {
}

void GeomUtilTest::test_calc_block_cone_defender() {
}

void GeomUtilTest::test_offset_to_line() {
}

void GeomUtilTest::test_offset_along_line() {
}

void GeomUtilTest::test_segment_near_line() {
}

void GeomUtilTest::test_intersection() {
}

void GeomUtilTest::test_vertex_angle() {
}

void GeomUtilTest::test_closet_point_time() {
}

