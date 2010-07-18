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
std::vector<size_t> dist_matching(const std::vector<Point>& v1, const std::vector<Point>& v2);

/**
 * Checks collinearity.
 */
bool collinear(const Point& a, const Point& b, const Point& c);

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
std::pair<Point, double> angle_sweep_circles(const Point& src, const Point& p1, const Point& p2, const std::vector<Point>& obstacles, const double& radius);

/**
 *sees whether the line segment intersects the rectangle
 */
bool line_seg_intersect_rectangle(Point seg[2], Point recA[4]);

/**
 *sees whether the point lies inside of the rectangle
 */
bool point_in_rectangle(Point pointA, Point recA[4]);

/**
 *returns a vector of all the points where the line segment intersects the circlecross product
 */
std::vector<Point> lineseg_circle_intersect(Point centre, double radius, Point segA, Point segB); 

/**
 *returns a vector of all the points where the line defined by two points intersects the circle
 */
std::vector<Point> line_circle_intersect(Point centre, double radius, Point segA, Point segB); 

/**
 * Clips a point to a rectangle boundary.
 */
Point clip_point(const Point& p, const Point& bound1, const Point& bound2);

/*
   OLD CODE STARTS HERE
 */

// ported code
// WARNING: does not work with parallel lines
// finds the intersection of 2 non-parallel lines
Point line_intersect(const Point &a, const Point &b, const Point &c, const Point &d);

// ported code
// WARNING: output is SIGNED indicating clockwise/counterclockwise direction
// signed line-point distance
double line_point_dist(const Point &p, const Point &a, const Point &b);

// ported code
// tests if 2 line segments crosses each other
// warning: LOOKS BROKEN
bool seg_crosses_seg(const Point &a1, const Point &a2, const Point &b1, const Point &b2);

/**
 * Ported code.
 * Reflects the ray incident on origin, with normal n
 */
Point reflect(const Point& v, const Point& n);

/**
 * Reflects the point p with the line a-b.
 */
Point reflect(const Point& a, const Point& b, const Point& p);

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
Point calc_block_side_pos(const Point& a, const Point& b, const Point& p, const double& radius, const double& thresh, const int side);

/**
 * there is a cone that shoots out from origin
 * the cone is bounded by direction vectors a and b
 * want to block this cone with circle of radius r
 * where to position the circle?
 * PRECONDITION: a-b must be counterclockwise from the origin.
 * PRECONDITION: cone has positive angle, i.e. not just a line.
 */
Point calc_block_cone(const Point &a, const Point &b, const double& radius);

Point calc_block_cone(const Point &a, const Point &b, const Point& p, const double& radius);

/**
 * Used for defender_blocks_goal
 * a = goal post side
 * c = ball position
 * g = goalie position
 * returns the other ray/cone boundary that is not blocked by goalie
 * I.e. if p is return value,
 * then points to the other side of line p-c is not covered by goalie.
 */
Point calc_block_other_ray(const Point& a, const Point& c, const Point& g);

// ported code
// a = goal post position
// b = other goal post position
// c = ball position
// g = goalie position
// checks if goalie blocks goal post
bool goalie_block_goal_post(const Point& a, const Point& b, const Point& c, const Point& g);

/**
 * a = goal post position
 * b = other goal post position
 * c = ball position
 * g = goalie position
 * finds a defender position to block the ball
 */
Point calc_block_cone_defender(const Point& a, const Point& b, const Point& c, const Point& g, const double& r);

#endif

