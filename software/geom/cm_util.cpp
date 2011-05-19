#include "geom/cm_util.h"
#include "geom/angle.h"
#include <cmath>

namespace {
	const double EPS = 1e-9;

	// used for lensq only
	const double EPS2 = EPS * EPS;

	// ported code
	int sign(double n) {
		return n > EPS ? 1 : (n < -EPS ? -1 : 0);
	}
}

double distance_to_line(Point x0, Point x1, Point p) {
	Point x;
	double t;

	t = ((p.x - x0.x) + (p.y - x0.y)) / (x1.x + x1.y);
	x = x0 + (x1 - x0) * t;

	return std::sqrt((x.x - p.x) * (x.x - p.x) + (x.y - p.y) * (x.y - p.y));
}

double offset_to_line(Point x0, Point x1, Point p) {
	Point n;

	// get normal to line
	n = (x1 - x0).perp().norm();

	return n.dot(p - x0);
}

double offset_along_line(Point x0, Point x1, Point p) {
	Point n, v;

	// get normal to line
	n = x1 - x0;
	n = n.norm();

	v = p - x0;

	return n.dot(v);
}

Point segment_near_line(Point a0, Point a1, Point b0, Point b1) {
	Point v, n, p;
	double dn, t;

	v = a1 - a0;
	n = (b1 - b0).norm();
	n = n.perp();

	dn = v.dot(n);
	if (std::fabs(dn) < EPS) {
		return a0;
	}

	t = -(a0 - b0).dot(n) / dn;
	// printf("t=%f dn=%f\n",t,dn);
	if (t < 0) {
		t = 0;
	}
	if (t > 1) {
		t = 1;
	}
	p = a0 + v * t;

	return p;
}

Point intersection(Point a1, Point a2, Point b1, Point b2) {
	Point a = a2 - a1;

	Point b1r = (b1 - a1).rotate(-a.orientation());
	Point b2r = (b2 - a1).rotate(-a.orientation());
	Point br = (b1r - b2r);

	return Point(b2r.x - b2r.y * (br.x / br.y), 0.0).rotate(a.orientation()) + a1;
}

double vertex_angle(Point a, Point b, Point c) {
	return angle_mod((a - b).orientation() - (c - b).orientation());
}

Point point_on_segment(Point x0, Point x1, Point p) {
	Point sx, sp, r;
	double f, l;

	sx = x1 - x0;
	sp = p - x0;

	f = sx.dot(sp);
	if (f <= 0.0) {
		return x0;  // also handles x0=x1 case
	}
	l = sx.lensq();
	if (f >= l) {
		return x1;
	}

	r = x0 + sx * (f / l);

	return r;
}

double closest_point_time(Point x1, Point v1, Point x2, Point v2) {
	Point v = v1 - v2;
	double sl = v.lensq();
	double t;

	if (sl < EPS) {
		return 0.0;  // parallel tracks, any time is ok.
	}
	t = -v.dot(x1 - x2) / sl;
	if (t < 0.0) {
		return 0.0;  // nearest time was in the past, now is closest point from now on.
	}
	return t;
}

