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
double angle_diff(double a, double b) __attribute__((const));

/**
 * Converts an angle in degrees into radians.
 *
 * \param[in] x the angle to convert.
 *
 * \return the result of the conversion.
 */
double degrees2radians(double x) __attribute__((const));

/**
 * Converts an angle in radians into degrees.
 *
 * \param[in] x the angle to convert.
 *
 * \return the result of the conversion.
 */
double radians2degrees(double x) __attribute__((const));

#endif

