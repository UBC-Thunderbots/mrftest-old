#ifndef GEOM_UTIL_H
#define GEOM_UTIL_H

#include "geom/point.h"
#include <vector>

/*
 * Misc geometry utility functions.
 * Contains code ported from 2009 version of AI.
 */

/**
 * Computes a minimum-total-distance bipartite matching between sets of points.
 *
 * \param[in] v1 the first set of points.
 *
 * \param[in] v2 the second set of points.
 *
 * \return the order of the matching, such that element <var>i</var> of input
 * \p v1 is matched with element \c order[<var>i</var>] of \p v2.
 */
std::vector<size_t> dist_matching(const std::vector<Point>& v1, const std::vector<Point>& v2);

/**
 * Checks collinearity.
 */
bool collinear(const Point& a, const Point& b, const Point& c);

/**
 * Performs an angle sweep. Suppose in this world, all objects are circles of
 * fixed radius. You are at point \p src, and you want to shoot a ray between \p
 * p1 and \p p2. This function calculates the largest open angle interval that
 * you can shoot.
 *
 * \pre \p p1 must be to the right of \p p2. In other words, if there is a counterclockwise ordering, \p p1 is before \p p2 from \p src's point of view.
 *
 * \pre The angle \p p1, \p src, \p p2 must not be greater than 180 degrees.
 *
 * \pre \p p must not be between \p p1 and \p p2.
 *
 * \param[in] src the location where you are standing.
 *
 * \param[in] p1 the location of the right-hand edge of the target area.
 *
 * \param[in] p2 the location of the left-hand edge of the target area.
 *
 * \param[in] obstacles the coordinates of the centres of the obstacles.
 *
 * \param[in] radius the radii of the obstacles.
 *
 * \return the best direction to shoot and the size of the angle centred around
 * that direction that is completely free of obstacles, or <code>(<var>p</var>,
 * 0)</code> for some unspecified <var>p</var> if there is no free path.
 */
std::pair<Point, double> angle_sweep_circles(const Point& src, const Point& p1, const Point& p2, const std::vector<Point>& obstacles, const double& radius);

/**
 * Checks whether a line segment intersects a rectangle.
 *
 * \param[in] seg the endpoints of the line segment.
 *
 * \param[in] recA the corners of the rectangle.
 *
 * \return \c true if any part of the segment lies inside the rectangle, or \c
 * false if the entire segment lies outside the rectangle.
 */
bool line_seg_intersect_rectangle(Point seg[2], Point recA[4]);

/**
 * Checks whether a point lies inside a rectangle.
 *
 * \param[in] pointA the point to check.
 *
 * \param[in] recA the corners of the rectangle.
 *
 * \return \c true if \p pointA lies inside the rectangle, or \c false if it
 * lies outside.
 */
bool point_in_rectangle(Point pointA, Point recA[4]);

/**
 * Finds the points of intersection between a circle and a line segment. There
 * may be zero, one, or two such points.
 *
 * \param[in] centre the centre of the circle.
 *
 * \param[in] radius the radius of the circle.
 *
 * \param[in] segA one end of the line segment.
 *
 * \param[in] segB the other end of the line segment.
 *
 * \return the points of intersection.
 */
std::vector<Point> lineseg_circle_intersect(Point centre, double radius, Point segA, Point segB); 

/**
 * Finds the points of intersection between a circle and a line. There may be
 * zero, one, or two such points.
 *
 * \param[in] centre the centre of the circle.
 *
 * \param[in] radius the radius of the circle.
 *
 * \param[in] segA one point on the line.
 *
 * \param[in] segB another point on the line.
 *
 * \return the points of intersection.
 */
std::vector<Point> line_circle_intersect(Point centre, double radius, Point segA, Point segB); 

/**
 * Clips a point to a rectangle boundary.
 *
 * \param[in] p the point to clip.
 *
 * \param[in] bound1 one corner of the rectangle.
 *
 * \param[in] bound2 the diagonally-opposite corner of the rectangle.
 *
 * \return the closest point to \p p that lies within the rectangle.
 */
Point clip_point(const Point& p, const Point& bound1, const Point& bound2);

/**
 * Computes the intersection of two lines.
 *
 * \pre The lines must be non-parallel.
 *
 * \param[in] a a point on the first line.
 *
 * \param[in] b another point on the first line.
 *
 * \param[in] c a point on the second line.
 *
 * \param[in] d another point on the second line.
 *
 * \return the point of intersection.
 */
Point line_intersect(const Point &a, const Point &b, const Point &c, const Point &d);

/**
 * Computes the distance from a point to a line.
 *
 * \param[in] p the point.
 *
 * \param[in] a a point on the line.
 *
 * \param[in] b another point on the line.
 *
 * \return the signed distance from the point to the line, with a negative
 * number indicating that the point is counterclockwise of the line and a
 * positive number indicating that the point is clockwise of the line.
 */
double line_point_dist(const Point &p, const Point &a, const Point &b);

/**
 * Checks whether two line segments intersect.
 *
 * \param[in] a1 one end of the first segment.
 *
 * \param[in] a2 the other end of the first segment.
 *
 * \param[in] b1 one end of the second segment.
 *
 * \param[in] b2 the other end of the second segment.
 *
 * \return \c true if the segments intersect, or \c false if not.
 */
bool seg_crosses_seg(const Point &a1, const Point &a2, const Point &b1, const Point &b2);

/**
 * Reflects a ray incident on origin given the normal of the reflecting plane.
 *
 * \param[in] v the incident ray to reflect.
 *
 * \param[in] n the normal vector to the reflecting plane.
 *
 * \return the reflected ray.
 */
Point reflect(const Point& v, const Point& n);

/**
 * Reflects a point across a line.
 *
 * \param[in] a a point on the line.
 *
 * \param[in] b another point on the line.
 *
 * \param[in] p the point to reflect.
 *
 * \return the reflection of \p p across the line.
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
 * Given a cone shooting from the origin, determines the location at which to
 * place a circle to block as much as possible of the cone.
 *
 * \pre The cone must have nonzero area.
 *
 * \pre \p b must be counterclockwise of \p a.
 *
 * \param[in] a the starting angle of the cone.
 *
 * \param[in] b the ending angle of the cone.
 *
 * \param[in] radius the radius of the circle with which to block the cone.
 *
 * \return the blocking position.
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

