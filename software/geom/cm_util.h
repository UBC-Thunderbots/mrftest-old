#ifndef GEOM_CM_UTIL_H
#define GEOM_CM_UTIL_H

#include "geom/point.h"
#include "geom/rect.h"
#include <vector>
#include <math.h>

#ifndef M_2PI
#define M_2PI 6.28318530717958647693
#endif


double bound(double x, double low, double high);

// returns distance from point p to line x0-x1
double distance_to_line(const Point x0, const Point x1, const Point p);

// returns perpendicular offset from line x0-x1 to point p
double offset_to_line(const Point x0, const Point x1, const Point p);

// returns perpendicular offset from line x0-x1 to point p
double offset_along_line(const Point x0, const Point x1, const Point p);

// returns nearest point on segment a0-a1 to line b0-b1
Point segment_near_line(const Point a0, const Point a1, const Point b0, const Point b1);

//
Point intersection(const Point a1, const Point a2, const Point b1, const Point b2);

// gives counterclockwise angle from <a-b> to <c-b>
double vertex_angle(const Point a, const Point b, const Point c);


//==== Generic functions =============================================//
// (work on 2d or 3d vectors)

// returns nearest point on line segment x0-x1 to point p
Point point_on_segment(const Point x0,const Point x1,const Point p);

// returns time of closest point of approach of two points
// moving along constant velocity vectors.
double closest_point_time(const Point x1, const Point v1, const Point x2, const Point v2);

#endif
