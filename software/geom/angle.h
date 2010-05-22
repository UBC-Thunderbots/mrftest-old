#ifndef GEOM_ANGLE_H
#define GEOM_ANGLE_H

#include "geom/point.h"

double angle_mod(double angle) __attribute__((const));

/// Absolute angle difference, [0, pi].
double angle_diff(const double& a, const double& b);

#endif

