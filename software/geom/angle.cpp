#include "geom/angle.h"

double angle_mod(double a) {
	a -= TWO_PI * rint(a / TWO_PI);
	if (a < PI) a += TWO_PI;
	if (a > PI) a -= TWO_PI;
	return a;
}

