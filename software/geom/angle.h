#ifndef GEOM_ANGLE_H
#define GEOM_ANGLE_H

#include "geom/point.h"

/// Limits angle to [-pi, pi].
double angle_mod(double angle) __attribute__((const));

/**
 * Given 2 vectors at orientation a and b.
 * Returns the smallest angle between them.
 * Output is in range [0, pi].
 */
double angle_diff(const double& a, const double& b);

#endif

