#include "geom/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

namespace {
	class GeomUtilTest : public CppUnit::TestFixture {
		CPPUNIT_TEST_SUITE(GeomUtilTest);
		CPPUNIT_TEST(test_collinear);
		CPPUNIT_TEST(test_line_intersect);
		CPPUNIT_TEST(test_seg_crosses_seg);
		CPPUNIT_TEST(test_vector_crosses_seg);
		CPPUNIT_TEST(test_vector_rect_intersect);
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

void GeomUtilTest::test_line_pt_dist(){
}
void GeomUtilTest::test_seg_pt_dist(){
}
void GeomUtilTest::test_proj_dist(){
}

void GeomUtilTest::test_point_in_triangle(){
	// this triangle lies in the first quatren of the field, we can rota
	Point p1(0,0);
	Point p2((rand()%100)/100, 0);
	Point p3( (rand()%100)/100, (rand()%100)/100 );
	Point p( (rand()%100)/100, (rand()%100)/100 );
	bool expected_val = false;
	if( p.y <= p3.y && p.x >= p.y/ p3.y* p3.x && p.x <= (p.y/ p3.y* (p3.x-p2.x) + p2.x) ){
		expected_val = true;			// so the point is inside the triangle, 
	}
	Angle rot = Angle::of_degrees( rand() );
	Point trans( 1.2, 1.2 );
	p1 = p1.rotate(rot);
	p2 = p2.rotate(rot);
	p3 = p3.rotate(rot);
	p = p.rotate(rot);
	p1 += trans;
	p2 += trans;
	p3 += trans;
	p += trans;
	bool calculated_val = point_in_triangle( p1, p2, p3, p );
	CPPUNIT_ASSERT_EQUAL( calculated_val, expected_val );
}

void GeomUtilTest::test_triangle_circle_intersect(){
}
void GeomUtilTest::test_dist_matching(){
	// not used anywhere
}
void GeomUtilTest::test_collinear(){
	std::srand(static_cast<unsigned int>(std::time(0)));
	for (unsigned int i = 0; i < 10; ++i) {
		Point v = Point::of_angle(Angle::of_degrees(std::rand() % 360)); // should be random number here
		Point pointA((std::rand() % 100) / 100.0, (std::rand() % 100) / 100.0);
		Point pointB = pointA + v * (std::rand() % 100) / 100.0;
		Point pointC = pointA - v * (std::rand() % 100) / 100.0;
		bool val = collinear(pointA, pointB, pointC);
		CPPUNIT_ASSERT(val);
	}
}

void GeomUtilTest::test_angle_sweep_circles(){
	// yes, this is used
}

void GeomUtilTest::test_angle_sweep_circles_all(){
}

void GeomUtilTest::test_line_seg_intersect_rectangle(){
}

void GeomUtilTest::test_point_in_rectangle(){
}

void GeomUtilTest::test_seg_buffer_boundaries(){
}

void GeomUtilTest::test_circle_boundaries(){
}

void GeomUtilTest::test_lineseg_point_dist(){
}

void GeomUtilTest::test_closet_lineseg_point(){
}

void GeomUtilTest::test_line_circle_intersect(){
}

void GeomUtilTest::test_line_circle_intersect2(){
}

void GeomUtilTest::test_line_rect_intersect(){
}

void GeomUtilTest::test_vector_rect_intersect(){
	std::cout << "========= Enter vector_rect_intersect Test =========" << std::endl;
	Rect rect( Point(1.0,1.0), Point(-1.0,-1.0) );
	Point pr1 = Point( ((rand()%200)-100) /100.0, 1.0 );
	Point pr2 = Point( ((rand()%200)-100) /100.0, -1.0 );
	Point pr3 = Point( 1.0, ((rand()%200)-100) /100.0 );
	Point pr4 = Point( -1.0, ((rand()%200)-100) /100.0 );
	Point pb( ((rand()%200)-100) /100.0, ((rand()%200)-100) /100.0 );
	Point pe1 = (pr1-pb).norm() + pr1;
	Point pe2 = (pr2-pb).norm() + pr2;
	Point pe3 = (pr3-pb).norm() + pr3;
	Point pe4 = (pr4-pb).norm() + pr4;
	Point found1 = vector_rect_intersect( rect, pb, pr1 );
	Point found2 = vector_rect_intersect( rect, pb, pr2 );
	Point found3 = vector_rect_intersect( rect, pb, pr3 );
	Point found4 = vector_rect_intersect( rect, pb, pr4 );
	
	// uncomment to print out some debugging info
	/*std::cout << " vectorA (" << pb.x << ", " << pb.y << ") " << std::endl;
	std::cout << " Intersect1 (" << pr1.x << ", " << pr1.y << ") " << " found1 (" << found1.x << ", " << found1.y << ") " << std::endl;
	std::cout << " Intersect2 (" << pr2.x << ", " << pr2.y << ") " << " found2 (" << found2.x << ", " << found2.y << ") " << std::endl;
	std::cout << " Intersect3 (" << pr3.x << ", " << pr3.y << ") " << " found3 (" << found3.x << ", " << found3.y << ") " << std::endl;
	std::cout << " Intersect4 (" << pr4.x << ", " << pr4.y << ") " << " found4 (" << found4.x << ", " << found4.y << ") " << std::endl;
	std::cout << " vectorB1 (" << pe1.x << ", " << pe1.y << ") " << std::endl;
	std::cout << " vectorB2 (" << pe2.x << ", " << pe2.y << ") " << std::endl;
	std::cout << " vectorB3 (" << pe3.x << ", " << pe3.y << ") " << std::endl;
	std::cout << " vectorB4 (" << pe4.x << ", " << pe4.y << ") " << std::endl;*/

	CPPUNIT_ASSERT( (found1 - pr1).len() < 0.001 );
	CPPUNIT_ASSERT( (found2 - pr2).len() < 0.001 );
	CPPUNIT_ASSERT( (found3 - pr3).len() < 0.001 );
	CPPUNIT_ASSERT( (found4 - pr4).len() < 0.001 );
}

void GeomUtilTest::test_clip_point(){
}

void GeomUtilTest::test_unique_line_intersect(){
}

void GeomUtilTest::test_line_intersect(){
	std::srand(static_cast<unsigned int>(std::time(0)));
	std::cout << "========= Enter line_intersect Test ========" << std::endl;

// should check for the the rare cases

	for( int i = 0; i < 10 ; i++ ){
		// generate three random points
		Point a1( rand()%200 /100.0, rand()%200 /100.0 );
		Point b1( rand()%200 /100.0, rand()%200 /100.0 );
		Point expected( rand()%200 /100.0, rand()%200 /100.0 );

// We do not know what the  tolorance of the function is, but we probabaly should check if segments overlap completely

		Point a2 = a1 + (expected - a1) * (1 + rand()%200/100.0 );
		Point b2 = b1 + (expected - b1) * (1 + rand()%200/100.0 );

		Point found = line_intersect( a1, a2, b1, b2 );
		
		// uncomment to print out some messages
		/*std::cout << "points are (" << a1.x << ", " << a1.y << ") ";
		std::cout << " (" << a2.x << ", " << a2.y << ") ";
		std::cout << " (" << b1.x << ", " << b1.y << ") ";
		std::cout << " (" << b2.x << ", " << b2.y << ") " << std::endl;
		std::cout << "expecting (" << expected.x << ", " << expected.y << ") " << std::endl;
		std::cout << "found (" << found.x << ", " << found.y << ") " << std::endl;*/
		
		CPPUNIT_ASSERT( (expected-found).len() < 0.0001 );
	}
}

void GeomUtilTest::test_line_point_dist(){
}

void GeomUtilTest::test_seg_crosses_seg(){
	std::srand(static_cast<unsigned int>(std::time(0)));
	std::cout << "========= Enter seg_crosses_seg Test ========" << std::endl;

// should check for the the rare cases

	for( int i = 0; i < 10 ; i++ ){
		// generate three random points
		Point a1( rand()%200 /100.0, rand()%200 /100.0 );
		Point b1( rand()%200 /100.0, rand()%200 /100.0 );
		Point i0( rand()%200 /100.0, rand()%200 /100.0 );

// We do not know what the  tolorance of the function is, but we probabaly should check if segments overlap completely

		bool a_over = rand()%2;
		bool b_over = rand()%2;
		
		Point a2 = a1 + (i0 - a1) * (1 + rand()%100/100.0 * (a_over?1:-1));	// the last part generate a number either bigger or smaller than 1 
		Point b2 = b1 + (i0 - b1) * (1 + rand()%100/100.0 * (b_over?1:-1));	// as a scaling factor for a2 and b2

		bool expected = a_over && b_over;
		bool found = seg_crosses_seg( a1, a2, b1, b2 );
	
		// uncomment to print out some messages
		/*std::cout << "points are (" << a1.x << ", " << a1.y << ") ";
		std::cout << " (" << a2.x << ", " << a2.y << ") ";
		std::cout << " (" << b1.x << ", " << b1.y << ") ";
		std::cout << " (" << b2.x << ", " << b2.y << ") ";
		std::cout << " (" << i0.x << ", " << i0.y << ") ";
		std::cout << " a_over " << (a_over?"true":"false") << " b_over " << (b_over?"true":"false") << std::endl;
		std::cout << "expecting " << (expected?"true":"false") << " found " << (found?"true":"false") << std::endl;*/
	
		CPPUNIT_ASSERT_EQUAL( expected, found );
	}
}

void GeomUtilTest::test_vector_crosses_seg(){
	std::srand(static_cast<unsigned int>(std::time(0)));
	std::cout << "========= Enter vector_crosses_seg Test ========" << std::endl;

// should check for the the rare cases

// case where vector faces segment but segment may/ may not be long enough
	for( int i = 0; i < 5 ; i++ ){
		// generate three random points
		Point a1( rand()%200 /100.0, rand()%200 /100.0 );
		Point b1( rand()%200 /100.0, rand()%200 /100.0 );
		Point i0( rand()%200 /100.0, rand()%200 /100.0 );

// We do not know what the  tolorance of the function is, but we probabaly should check if segments overlap completely

		bool expected = rand()%2;
		
		Point a2 = a1 + (i0 - a1).norm(); 
		Point b2 = b1 + (i0 - b1) * (1 + rand()%100/100.0 * (expected?1:-1));	// as a scaling factor for b2

		bool found = vector_crosses_seg( a1, a2, b1, b2 );
	
		// uncomment to print out some messages
		/*std::cout << "points are (" << a1.x << ", " << a1.y << ") ";
		std::cout << " (" << a2.x << ", " << a2.y << ") ";
		std::cout << " (" << b1.x << ", " << b1.y << ") ";
		std::cout << " (" << b2.x << ", " << b2.y << ") ";
		std::cout << " (" << i0.x << ", " << i0.y << ") ";
		std::cout << "expecting " << (expected?"true":"false") << " found " << (found?"true":"false") << std::endl;*/
	
		CPPUNIT_ASSERT_EQUAL( expected, found );
	}

// case where vector does not face segment
	for( int i = 0; i < 5 ; i++ ){
		// generate three random points
		Point a1( rand()%200 /100.0, rand()%200 /100.0 );
		Point b1( rand()%200 /100.0, rand()%200 /100.0 );
		Point i0( rand()%200 /100.0, rand()%200 /100.0 );

// We do not know what the  tolorance of the function is, but we probabaly should check if segments overlap completely

		bool expected = false;
		
		Point a2 = a1 - (i0 - a1).norm(); 
		Point b2 = b1 + (i0 - b1) * (1 + rand()%100/100.0 );	// as a scaling factor for b2, make sure it is long enough

		bool found = vector_crosses_seg( a1, a2, b1, b2 );
	
		// uncomment to print out some messages
		/*std::cout << "points are (" << a1.x << ", " << a1.y << ") ";
		std::cout << " (" << a2.x << ", " << a2.y << ") ";
		std::cout << " (" << b1.x << ", " << b1.y << ") ";
		std::cout << " (" << b2.x << ", " << b2.y << ") ";
		std::cout << " (" << i0.x << ", " << i0.y << ") ";
		std::cout << "expecting " << (expected?"true":"false") << " found " << (found?"true":"false") << std::endl;*/
	
		CPPUNIT_ASSERT_EQUAL( expected, found );
	}
}

void GeomUtilTest::test_reflect(){
}

void GeomUtilTest::test_reflect2(){
}

void GeomUtilTest::test_calc_block_cone(){
}

void GeomUtilTest::test_calc_block_cone2(){
}

void GeomUtilTest::test_calc_block_cone3(){
}

void GeomUtilTest::test_calc_block_other_ray(){
}

void GeomUtilTest::test_calc_goalie_block_goal_post(){
}

void GeomUtilTest::test_calc_block_cone_defender(){
}

void GeomUtilTest::test_offset_to_line(){
}

void GeomUtilTest::test_offset_along_line(){
}

void GeomUtilTest::test_segment_near_line(){
}

void GeomUtilTest::test_intersection(){
}

void GeomUtilTest::test_vertex_angle(){
}

void GeomUtilTest::test_closet_point_time(){
}


