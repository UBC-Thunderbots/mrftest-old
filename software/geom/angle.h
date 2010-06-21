#ifndef GEOM_ANGLE_H
#define GEOM_ANGLE_H

#include "geom/point.h"

#include <cmath>

/// Limits angle to [-pi, pi].
double angle_mod(double angle) __attribute__((const));

/**
 * Given 2 vectors at orientation a and b.
 * Returns the smallest angle between them.
 * Output is in range [0, pi].
 */
double angle_diff(const double& a, const double& b);

static inline double degrees2radians(const double& x) {
	return x * M_PI / 180.0;
}

static inline double radians2degrees(const double& x) {
	return x * 180.0 / M_PI;
}

#endif

