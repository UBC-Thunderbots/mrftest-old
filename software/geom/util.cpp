#include "geom/util.h"

#include <cassert>

namespace {
	const double eps = 1e-9;
}

point clip_point(const point& p, const point& bound1, const point& bound2) {
	const double minx = std::min(bound1.x, bound2.x);
	const double miny = std::min(bound1.y, bound2.y);
	const double maxx = std::max(bound1.x, bound2.x);
	const double maxy = std::max(bound1.y, bound2.y);
	point ret = p;
	if (p.x < minx) ret.x = minx;
	else if (p.x > maxx) ret.x = maxx;      
	if (p.y < miny) ret.y = miny;
	else if (p.y > maxy) ret.y = maxy;
	return ret;
}

// ported code
point line_intersect(const point &a, const point &b, const point &c, const point &d) {
	assert((d - c).cross(b - a) != 0);
	return a + (a - c).cross(d - c) / (d - c).cross(b - a) * (b - a);
}

// ported code
point calc_block_ray(const point &a, const point &b, const double& radius) {
	// unit vector and bisector
	point au = a / a.len();
	point c = au + b / b.len();
	// use similar triangle
	return c * (radius / std::fabs(au.cross(c)));
}

// ported code
double line_point_dist(const point &p, const point &a, const point &b) {
	return (p - a).cross(b - a) / (b - a).len();
}

// ported code
inline double sign(const double n) {
	return n > eps ? 1 : (n < -eps ? -1 : 0);
}

// ported code
bool seg_crosses_seg(const point& a1, const point& a2, const point &b1, const point &b2) {
	return sign((a2 - a1).cross(b1 - a1))
		* sign((a2 - a1).cross(b2 - a1)) <= 0 &&
		sign((b2 - b1).cross(a1 - b1))
		* sign((b2 - b1).cross(a2 - b1)) <= 0;
}

// ported code
point reflect(const point&v, const point& n) {
	point normal = n / n.len();
	return 2 * v.dot(normal) * normal - v;
}

// ported code
point calcBlockOtherRay(const point& a, const point& c, const point& g) {
	return reflect(a - c, g - c);
}

// ported code
bool goalieBlocksGoalPost(const point& a, const point& b, const point& c, const point& g) {
	point R = reflect(a - c, g - c);
	return (R.cross(b - c) < -eps);
}

// ported code
point defender_blocks_goal(const point& a, const point& b, const point& c, const point& g, const double& r) {
	point R = reflect(a - c, g- c);
	return calc_block_ray(R, b - c, r) + c;
}

