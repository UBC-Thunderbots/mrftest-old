#ifndef GEOM_POINT_H
#define GEOM_POINT_H

#include <complex>

//
// A point in 2D space.
//
typedef std::complex<double> point;

//
// Rotates a point about the origin.
//
// Parameters:
//  pt
//   the point to rotate
//
//  rot
//   the number of radians to rotate, measured counterclockwise
//
point rotate(const point &pt, double rot);

#endif

