#include "geom/rect.h"
#include "geom/util.h"
#include <algorithm>
#include <cmath>

bool Rect::expand(double amount) {
	if (diag.x < -2 * amount || diag.y < -2 * amount) {
		return false;
	}
	Point add(amount, amount);
	min_corner -= add;
	diag += 2 * add;
	return true;
}

double Rect::dist_to_boundary(Point p) {
	double dist = 10e9; // approx of infinity
	for (unsigned int i = 0; i < 4; i++) {
		Point a = operator[](i);
		Point b = operator[](i + 1);
		dist = std::min(dist, lineseg_point_dist(p, a, b));
	}
	return dist;
}

