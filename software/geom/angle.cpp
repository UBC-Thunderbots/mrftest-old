#include "geom/angle.h"
#include <cmath>

const double PI = std::acos(-1.0);

double angle_mod(double a) {
	a -= 2 * PI * rint(a / (2 * PI));
	if (a < PI) a += 2 * PI;
	if (a > PI) a -= 2 * PI;
	return a;
}

