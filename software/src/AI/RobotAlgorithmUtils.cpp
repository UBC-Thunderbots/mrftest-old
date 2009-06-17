#include "AI/RobotAlgorithmUtils.h"

#include <utility>
#include <cmath>
#include <cassert>

typedef std::pair<Vector2, Vector2> Line;

const double eps = 1e-9;

Vector2 line_intersect(const Vector2 &a, const Vector2 &b, const Vector2 &c, const Vector2 &d) {
	assert((d - c).cross(b - a) != 0);
	return a + (a - c).cross(d - c) / (d - c).cross(b - a) * (b - a);
}

Vector2 calc_block_ray(const Vector2 &a, const Vector2 &b, const double radius) {
	// unit vector and bisector
	Vector2 au = a / a.length();
	Vector2 c = au + b / b.length();
	// use similar triangle
	return c * (radius / std::fabs(au.cross(c)));
}

double line_point_dist(const Vector2 &p, const Vector2 &a, const Vector2 &b) {
	return (p - a).cross(b - a) / (b - a).length();
}

inline double sign(const double n) {
	return n > eps ? 1 : (n < -eps ? -1 : 0);
}

bool seg_crosses_seg(const Vector2& a1, const Vector2& a2, const Vector2 &b1, const Vector2 &b2) {
	return sign((a2 - a1).cross(b1 - a1))
		* sign((a2 - a1).cross(b2 - a1)) <= 0 &&
		sign((b2 - b1).cross(a1 - b1))
		* sign((b2 - b1).cross(a2 - b1)) <= 0;
}

