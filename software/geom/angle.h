#ifndef GEOM_ANGLE_H
#define GEOM_ANGLE_H

#include "geom/point.h"

#include <cmath>

/**
 * Limits an angle to [−π, π].
 *
 * \param[in] angle the angle to limit.
 *
 * \return the clamped angle.
 */
double angle_mod(double angle) __attribute__((const));

/**
 * Given 2 vectors, returns the smallest angle between them.
 *
 * \param[in] a the orientation of the first vector.
 *
 * \param[in] b the orientation of the second vector.
 *
 * \return the angle between \p a and \p b, in the range [0, π].
 */
double angle_diff(const double& a, const double& b);

/**
 * Converts an angle in degrees into radians.
 *
 * \param[in] x the angle to convert.
 *
 * \return the result of the conversion.
 */
static inline double degrees2radians(const double& x) {
	return x * M_PI / 180.0;
}

/**
 * Converts an angle in radians into degrees.
 *
 * \param[in] x the angle to convert.
 *
 * \return the result of the conversion.
 */
static inline double radians2degrees(const double& x) {
	return x * 180.0 / M_PI;
}

#endif

