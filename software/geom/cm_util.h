#ifndef GEOM_CM_UTIL_H
#define GEOM_CM_UTIL_H

#include "geom/point.h"

/*
 * returns distance from point p to line x0-x1
 */
double distance_to_line(Point x0, Point x1, Point p);

/**
 * returns perpendicular offset from line x0-x1 to point p
 */
double offset_to_line(Point x0, Point x1, Point p);

/**
 * returns perpendicular offset from line x0-x1 to point p
 */
double offset_along_line(Point x0, Point x1, Point p);

/*
 * returns nearest point on segment a0-a1 to line b0-b1
 */
Point segment_near_line(Point a0, Point a1, Point b0, Point b1);

/**
 * intersection of two segments?
 */
Point intersection(Point a1, Point a2, Point b1, Point b2);

/**
 * gives counterclockwise angle from <a-b> to <c-b>
 */
double vertex_angle(Point a, Point b, Point c);

/**
 * returns nearest point on line segment x0-x1 to point p
 */
Point point_on_segment(Point x0, Point x1, Point p);

/**
 * returns time of closest point of approach of two points
 * moving along constant velocity vectors.
 */
double closest_point_time(Point x1, Point v1, Point x2, Point v2);

#endif

