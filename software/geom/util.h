#ifndef GEOM_UTIL_H
#define GEOM_UTIL_H

#include "geom/point.h"
#include <vector>

/**
 *  Misc geometry utility functions.
 *  Contains code ported from 2009 version of AI.
 */

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
// WARNING: does not work with parallel lines
// there is a ray that shoots out from origin
// the ray is bounded by direction vectors a and b
// want to block this ray with circle of radius r
// where to position the circle?
point calc_block_ray(const point &a, const point &b, const double& radius);

// ported code
// WARNING: output is SIGNED indicating clockwise/counterclockwise direction
// signed line-point distance
double line_point_dist(const point &p, const point &a, const point &b);

// ported code
// tests if 2 line segments crosses each other
bool seg_crosses_seg(const point &a1, const point &a2, const point &b1, const point &b2);

// ported code
// reflects the ray r incident on origin, with normal n
point reflect(const point&v, const point& n);

// ported code
// a = goal post position
// c = ball position
// g = goalie position
// returns the other ray that is not blocked by goalie
point calcBlockOtherRay(const point& a, const point& c, const point& g);

// ported code
// a = goal post position
// b = other goal post position
// c = ball position
// g = goalie position
// checks if goalie blocks goal post
bool goalieBlocksGoalPost(const point& a, const point& b, const point& c, const point& g);

// ported code
// a = goal post position
// b = other goal post position
// c = ball position
// g = goalie position
// finds a defender position to block the ball
point defender_blocks_goal(const point& a, const point& b, const point& c, const point& g, const double& r);

#endif

