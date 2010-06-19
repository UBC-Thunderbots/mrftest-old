#ifndef GEOM_UTIL_H
#define GEOM_UTIL_H

#include "geom/point.h"
#include <vector>

/**
 *  Misc geometry utility functions.
 *  Contains code ported from 2009 version of AI.
 */

/**
 * Matches points of two different vectors.
 * Returns ordering of the matching such that the total distance is minimized.
 * If order is the returned vector.
 * Then the i element of v1 is matched with order[i] element of v2.
 */
std::vector<size_t> dist_matching(const std::vector<point>& v1, const std::vector<point>& v2);

/**
 * Checks collinearity.
 */
bool collinear(const point& a, const point& b, const point& c);

/**
 * Angle sweep.
 * Suppose in this world, all objects are circles of fixed radius.
 * You are at point src, and you want to shoot a ray between p1 and p2.
 * Calculates the largest open angle interval that you can shoot.
 * Returns direction, angle pair
 * angle is 0 if no answer is found, or input is invalid.
 * direction is normalized
 *
 * Preconditions:
 * - p1 must be to the right of p2. In other words, if there is a counterclockwise ordering, p1 is before p2 from p's point of view.
 * - the angle p1, p, p2 must not be greater than 180 degrees.
 * - p is not between p1 and p2.
 */
std::pair<point, double> angle_sweep_circles(const point& src, const point& p1, const point& p2, const std::vector<point>& obstacles, const double& radius);

/**
 *sees whether the line segment intersects the rectangle
 */
bool line_seg_intersect_rectangle(point seg[2], point recA[4]);

/**
 *sees whether the point lies inside of the rectangle
 */
bool point_in_rectangle(point pointA, point recA[4]);

/**
 *returns a vector of all the points where the line segment intersects the circlecross product
 */
std::vector<point> lineseg_circle_intersect(point centre, double radius, point segA, point segB); 

/**
 *returns a vector of all the points where the line defined by two points intersects the circle
 */
std::vector<point> line_circle_intersect(point centre, double radius, point segA, point segB); 

/**
 * Clips a point to a rectangle boundary.
 */
point clip_point(const point& p, const point& bound1, const point& bound2);

/*
   OLD CODE STARTS HERE
 */

// ported code
// WARNING: does not work with parallel lines
// finds the intersection of 2 non-parallel lines
point line_intersect(const point &a, const point &b, const point &c, const point &d);

// ported code
// WARNING: output is SIGNED indicating clockwise/counterclockwise direction
// signed line-point distance
double line_point_dist(const point &p, const point &a, const point &b);

// ported code
// tests if 2 line segments crosses each other
// warning: LOOKS BROKEN
bool seg_crosses_seg(const point &a1, const point &a2, const point &b1, const point &b2);

/**
 * Ported code.
 * Reflects the ray incident on origin, with normal n
 */
point reflect(const point& v, const point& n);

/**
 * Reflects the point p with the line a-b.
 */
point reflect(const point& a, const point& b, const point& p);

/**
 * Suppose there is a cone that shoots down from the ball.
 * Want to cover as much angle as possible.
 *
 * a,b = Goalpost sides; a is the left, b is the right.
 * p = ball location
 * r = Robots radius
 * Defend a particular side:
 * (not yet implemented) 0 = full center block, will cover half line
 * 1 = left half, will touch half line
 * 2 = right half, will touch half line
 * Defender cannoot be more than thresh distance away from the a-b line.
 */
point calc_block_side_pos(const point& a, const point& b, const point& p, const double& radius, const double& thresh, const int side);

/**
 * there is a cone that shoots out from origin
 * the cone is bounded by direction vectors a and b
 * want to block this cone with circle of radius r
 * where to position the circle?
 * PRECONDITION: a-b must be counterclockwise from the origin.
 * PRECONDITION: cone has positive angle, i.e. not just a line.
 */
point calc_block_cone(const point &a, const point &b, const double& radius);
static inline point calc_block_cone(const point &a, const point &b, const point& p, const double& radius) {
	return p + calc_block_cone(a - p, b - p, radius);
}

/**
 * Used for defender_blocks_goal
 * a = goal post side
 * c = ball position
 * g = goalie position
 * returns the other ray/cone boundary that is not blocked by goalie
 * I.e. if p is return value,
 * then points to the other side of line p-c is not covered by goalie.
 */
point calc_block_other_ray(const point& a, const point& c, const point& g);

// ported code
// a = goal post position
// b = other goal post position
// c = ball position
// g = goalie position
// checks if goalie blocks goal post
bool goalie_block_goal_post(const point& a, const point& b, const point& c, const point& g);

/**
 * a = goal post position
 * b = other goal post position
 * c = ball position
 * g = goalie position
 * finds a defender position to block the ball
 */
point calc_block_cone_defender(const point& a, const point& b, const point& c, const point& g, const double& r);

#endif

