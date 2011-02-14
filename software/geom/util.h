#ifndef GEOM_UTIL_H
#define GEOM_UTIL_H

#include "geom/point.h"
#include "geom/rect.h"
#include <vector>

/*
 * Misc geometry utility functions.
 * Contains code ported since 2009.
 *
 * PLEASE FOLLOW THE DEFINITION:
 * - LINE a line of infinite length
 * - SEGMENT a line of finite length
 */

/**
 * Computes a minimum-total-distance bipartite matching between sets of points.
 *
 * \param[in] v1 the first set of points.
 *
 * \param[in] v2 the second set of points.
 *
 * \return the order of the matching, such that element <var>i</var> of input \p v1 is matched with element \c order[<var>i</var>] of \p v2.
 */
std::vector<size_t> dist_matching(const std::vector<Point> &v1, const std::vector<Point> &v2);

/**
 * Checks if 3 points are collinear.
 *
 * \param[in] a a point
 *
 * \param[in] b a point
 *
 * \param[in] c a point
 *
 * \returns true if any two points are within EPS distance to each other.
 */
bool collinear(const Point &a, const Point &b, const Point &c);

/**
 * Performs an angle sweep.
 * Suppose in this world, all objects are circles of fixed radius.
 * You are at point \p src, and you want to shoot a ray between \p p1 and \p p2.
 * This function calculates the largest open angle interval that you can shoot.
 *
 * \pre \p p1 must be to the right of \p p2.
 * In other words, if there is a counterclockwise ordering, \p p1 is before \p p2 from \p src's point of view.
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
 * \return the best direction to shoot and the size of the angle centred around that direction that is completely free of obstacles,
 * or <code>(<var>p</var>, 0)</code> for some unspecified <var>p</var> if there is no free path.
 */
std::pair<Point, double> angle_sweep_circles(const Point &src, const Point &p1, const Point &p2, const std::vector<Point> &obstacles, const double &radius);

/**
 * Checks whether a line segment intersects a rectangle.
 *
 * \param[in] seg the endpoints of the line segment.
 *
 * \param[in] recA the corners of the rectangle.
 *
 * \return \c true if any part of the segment lies inside the rectangle, or \c false if the entire segment lies outside the rectangle.
 */
bool line_seg_intersect_rectangle(const Point seg[2], const Point recA[4]);

/**
 * Checks whether a point lies inside a rectangle.
 *
 * \param[in] pointA the point to check.
 *
 * \param[in] recA the corners of the rectangle.
 *
 * \return \c true if \p pointA lies inside the rectangle, or \c false if it lies outside.
 */
bool point_in_rectangle(const Point &pointA, const Point recA[4]);
/**
 * returns a list of points that lie exactle "buffer" distance awaw from the line seg
 *
 */
std::vector<Point> seg_buffer_boundaries(const Point &a, const Point &b, double buffer, int num_points);

/**
 * returns a list of points that lie on the border of the circle
 */
std::vector<Point> circle_boundaries(const Point &centre, double radius, int num_points);

/**
 * Finds the distance between and a line segment and a point.
 *
 * \param[in] centre the point.
 *
 * \param[in] segA one end of the line segment.
 *
 * \param[in] segB the other end of the line segment.
 *
 * \return the distance between line seg and point.
 */
double lineseg_point_dist(const Point &centre, const Point &segA, const Point &segB);

/**
 * Finds the points of intersection between a circle and a line.
 * There may be zero, one, or two such points.
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
std::vector<Point> line_circle_intersect(const Point &centre, double radius, const Point &segA, const Point &segB);

/**
 * Finds the points of intersection between a circle and a line.
 * There may be zero, one, or two such points.
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
std::vector<Point> line_circle_intersect(const Point &centre, double radius, const Point &segA, const Point &segB);

/**
 * Finds the points of intersection between a circle and a line.
 * There may be zero, one, or two such points.
 *
 * \param[in] r the rectangle.
 *
 * \param[in] segA one point on the line.
 *
 * \param[in] segB another point on the line.
 *
 * \return the points of intersection.
 */
std::vector<Point> line_rect_intersect(const Rect &r, const Point &segA, const Point &segB);


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
Point clip_point(const Point &p, const Point &bound1, const Point &bound2);

/**
 * Computes whether there is a unique intersection of two lines.
 *
 * \param[in] a a point on the first line.
 *
 * \param[in] b another point on the first line.
 *
 * \param[in] c a point on the second line.
 *
 * \param[in] d another point on the second line.
 *
 * \return whether there is one and only one answer
 */
bool unique_line_intersect(const Point &a, const Point &b, const Point &c, const Point &d);

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
 * \return the signed distance from the point to the line,
 * with a negative number indicating that the point is counterclockwise of the line and a positive number indicating that the point is clockwise of the line.
 */
double line_point_dist(const Point &p, const Point &a, const Point &b);

double seg_seg_distance(const Point &a, const Point &b, const Point &c, const Point &d);

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
 * \pre the normal vector cannot have length smaller than EPS.
 *
 * \param[in] v the incident ray to reflect.
 *
 * \param[in] n the vector normal to the reflecting plane.
 *
 * \return the reflected ray.
 */
Point reflect(const Point &v, const Point &n);

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
Point reflect(const Point &a, const Point &b, const Point &p);

/**
 * Given a cone shooting from the origin, determines the furthest location from the origin, at which to place a circle to block the cone.
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
Point calc_block_cone(const Point &a, const Point &b, const double &radius);

/**
 * Given a cone shooting from a point P, determines the furthest location from P, at which to place a circle to block the cone.
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
 * \param[in] p the source of the cone.
 *
 * \return the blocking position.
 */
Point calc_block_cone(const Point &a, const Point &b, const Point &p, const double &radius);

/**
 * Used for defender_blocks_goal
 * a = goal post side
 * c = ball position
 * g = goalie position
 * returns the other ray/cone boundary that is not blocked by goalie
 * I.e. if p is return value,
 * then points to the other side of line p-c is not covered by goalie.
 */
Point calc_block_other_ray(const Point &a, const Point &c, const Point &g);

/**
 * Ported code
 * Checks if goalie blocks goal post.
 *
 * \param[in] a goal post position
 *
 * \param[in] b other goal post position
 *
 * \param[in] c ball position
 *
 * \param[in] g goalie position
 */
bool goalie_block_goal_post(const Point &a, const Point &b, const Point &c, const Point &g);

/**
 * Calculates a defender position to block the ball.
 * Warning: I don't know what orientation the goal posts have to be.
 *
 * \pre the goalie is between the two goal posts, as seen from the ball.
 *
 * \param[in] a ??? goal post position
 *
 * \param[in] b ??? other goal post position
 *
 * \param[in] c ball position
 *
 * \param[in] g goalie position
 */
Point calc_block_cone_defender(const Point &a, const Point &b, const Point &c, const Point &g, const double &r);

#endif

