#include "geom/angle.h"
#include <cmath>

namespace {
	const double PI2 = M_PI * 2;
}

double angle_mod(double a) {
	// Bring the angle to [−2π, 2π].
	a = std::fmod(a, PI2);
	// Bring the angle to [0, 2π].
	if (a < 0) {
		a += PI2;
	}
	// Bring the angle to [−π, π].
	if (a > M_PI) {
		a -= PI2;
	}
	return a;
}

double angle_diff(double a, double b) {
	// Bring the angle to [−2π, 2π].
	double diff = std::fmod(b - a, PI2);
	// Bring the angle to [0, 2π].
	if (diff < 0) {
		diff += PI2;
	}
	// Bring the angle to [0, π].
	if (diff > M_PI) {
		diff = PI2 - diff;
	}
	return diff;
}

double degrees2radians(double x) {
	return x * M_PI / 180.0;
}

double radians2degrees(double x) {
	return x * 180.0 / M_PI;
}

