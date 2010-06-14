#include "geom/angle.h"
#include <cmath>

namespace {
	const double PI2 = M_PI * 2;
}

double angle_mod(double a) {
	/*
	a -= 2 * M_PI * rint(a / (PI2));
	if (a < M_PI) a += PI2;
	if (a > M_PI) a -= PI2;
	return a;
	*/
	a = fmod(a, PI2); // [0, pi]
	if (a > M_PI) a -= M_PI;
	return a;
}

double angle_diff(const double& a, const double& b) {
	// return fabs(angle_mod(a - b));
	double diff = fmod(b - a, PI2); // [0, 2pi]
	if (diff > M_PI) diff = PI2 - diff;
	return diff;
}

