#include "geom/angle.h"
#include <cmath>

double angle_mod(double a) {
	a -= 2 * M_PI * rint(a / (2 * M_PI));
	if (a < M_PI) a += 2 * M_PI;
	if (a > M_PI) a -= 2 * M_PI;
	return a;
}

