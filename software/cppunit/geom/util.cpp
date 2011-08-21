#include "geom/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

namespace {
	class GeomUtilTest : public CppUnit::TestFixture {
		CPPUNIT_TEST_SUITE(GeomUtilTest);
		CPPUNIT_TEST(test_collinear);
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
		Point pointA((std::rand() % 100) / 100, (std::rand() % 100) / 100);
		Point pointB = pointA + v * (std::rand() % 100) / 100;
		Point pointC = pointA - v * (std::rand() % 100) / 100;
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
}

void GeomUtilTest::test_clip_point(){
}

void GeomUtilTest::test_unique_line_intersect(){
}

void GeomUtilTest::test_line_intersect(){
}

void GeomUtilTest::test_line_point_dist(){
}

void GeomUtilTest::test_seg_crosses_seg(){
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


