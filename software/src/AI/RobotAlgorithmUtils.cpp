#include <iostream>

#include "AI/RobotAlgorithmUtils.h"

typedef std::pair<Vector2, Vector2> Line;

const double eps = 1e-9;

Vector2 line_intersect(const Vector2& a, const Vector2& b,
	const Vector2& c, const Vector2& d) {
	if(cross(d - c, b - a) == 0) {
		std::cerr << " lines are parallel " << std::endl;
	}
	return a + cross(a - c, d - c) / cross(d - c, b - a) * (b - a);
}

Vector2 calc_block_ray(const Vector2& a, const Vector2& b,
		const double radius) {
	// unit vector and bisector
	Vector2 au = a / a.length();
	Vector2 c = au + b / b.length();
	// use similar triangle
	return c * (radius / std::abs(cross(au, c)));
}

double line_point_dist(const Vector2& p,
		const Vector2& a, const Vector2& b) {
	return cross(p - a, b - a) / length(b - a);
}

inline double sign(const double n) {
	return n > eps ? 1 : (n < -eps ? -1 : 0);
}

bool seg_crosses_seg(const Vector2& a1, const Vector2& a2,
	const Vector2& b1, const Vector2& b2) {
	return sign(cross(a2 - a1, b1 - a1))
		* sign(cross(a2 - a1, b2 - a1)) <= 0 &&
		sign(cross(b2 - b1, a1 - b1))
		* sign(cross(b2 - b1, a2 - b1)) <= 0;
}

// use hungarian matching
std::vector<int> task_matching(const std::vector<std::vector<double> >& cost) {
	const int N = cost.size();
	// TODO
}

