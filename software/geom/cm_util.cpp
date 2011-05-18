#include "geom/cm_util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
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



double bound(double x, double low, double high){
  	if(x < low ) x = low;
  	if(x > high) x = high;
  	return(x);
}

// returns distance from point p to line x0-x1
double distance_to_line(const Point x0, const Point x1, const Point p){
  	Point x;
  	double t;

  	t = ((p.x - x0.x) + (p.y - x0.y)) / (x1.x + x1.y);
  	x = x0 + (x1 - x0) * t;

  	// printf("dist:(%f,%f)-(%f,%f)\n",x.x,x.y,p.x,p.y);

  	//return(distance(x,p));
	return sqrt((x.x - p.x)*(x.x - p.x) + (x.y - p.y)*(x.y - p.y));
}

// returns perpendicular offset from line x0-x1 to point p
double offset_to_line(const Point x0, const Point x1, const Point p){
  	Point n;

  	// get normal to line
  	n = (x1 - x0).perp().norm();

  	return(n.dot(p - x0));
}

// returns perpendicular offset from line x0-x1 to point p
double offset_along_line(const Point x0, const Point x1, const Point p){
  	Point n,v;

  	// get normal to line
  	n = x1 - x0;
  	n = n.norm();

  	v = p - x0;

  	return(n.dot(v));
}

// returns nearest point on segment a0-a1 to line b0-b1
Point segment_near_line(const Point a0, const Point a1, const Point b0, const Point b1){
  	Point v,n,p;
  	double dn,t;

  	v = a1-a0;
  	n = (b1-b0).norm();
  	n = n.perp();

  	dn = v.dot(n);
  	if(fabs(dn) < EPS) return(a0);

  	t = -(a0-b0).dot(n) / dn;
  	// printf("t=%f dn=%f\n",t,dn);
  	if(t < 0) t = 0;
  	if(t > 1) t = 1;
  	p = a0 + v*t;

  	return(p);
}

//
Point intersection(const Point a1, const Point a2, const Point b1, const Point b2){
  	Point a = a2 - a1;

  	Point b1r = (b1 - a1).rotate(-a.orientation());
  	Point b2r = (b2 - a1).rotate(-a.orientation());
  	Point br = (b1r - b2r);

  	return Point(b2r.x - b2r.y * (br.x / br.y), 0.0).rotate(a.orientation()) + a1;
}

// gives counterclockwise angle from <a-b> to <c-b>
double vertex_angle(const Point a, const Point b, const Point c){
  	return(angle_mod((a-b).orientation() - (c-b).orientation()));
}


//==== Generic functions =============================================//
// (work on 2d or 3d vectors)

// returns nearest point on line segment x0-x1 to point p
Point point_on_segment(const Point x0,const Point x1,const Point p){
  	Point sx,sp,r;
  	double f,l;

  	sx = x1 - x0;
  	sp = p  - x0;

  	f = sx.dot(sp);
  	if(f <= 0.0) return(x0); // also handles x0=x1 case

  	l = sx.lensq();
  	if(f >= l) return(x1);

  	r = x0 + sx * (f / l);

  	return(r);
}

// returns time of closest point of approach of two points
// moving along constant velocity vectors.
double closest_point_time(const Point x1, const Point v1, const Point x2, const Point v2){
	Point v  = v1 - v2;
  	double sl = v.lensq();
  	double t;

  	if(sl < EPS) return(0.0); // parallel tracks, any time is ok.

  	t = -v.dot(x1 - x2) / sl;
  	if(t < 0.0) return(0.0); // nearest time was in the past, now is closest point from now on.

  	return(t);
}
