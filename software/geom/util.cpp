#include "geom/util.h"
#include "geom/angle.h"

#include <cassert>
#include <algorithm>
#include <iostream>

namespace {

	const double EPS = 1e-9;

	// used for lensq only
	const double EPS2 = EPS * EPS;
}

std::vector<size_t> dist_matching(const std::vector<point>& v1, const std::vector<point>& v2) {
	if (v1.size() > 5) {
#warning TODO: use hungarian matching
		std::cerr << "geom: WARNING, hungarian not used yet" << std::endl;
	}
	std::vector<size_t> order(v2.size());
	for (size_t i = 0; i < v2.size(); ++i) order[i] = i;
	std::vector<size_t> best = order;
	double bestscore = 1e99;
	const size_t N = std::min(v1.size(), v2.size());
	do {
		double score = 0;
		for (size_t i = 0; i < N; ++i) {
			score += (v1[i] - v2[order[i]]).len();
		}
		if (score < bestscore) {
			bestscore = score;
			best = order;
		}
	} while(std::next_permutation(order.begin(), order.end()));
	return best;
}

std::pair<point, double> angle_sweep_circles(const point& src, const point& p1, const point& p2, const std::vector<point>& obstacles, const double& radius) {
	// default value to return if nothing is valid
	point bestshot = (p1 + p2) * 0.5;
	const double offangle = (p1 - src).orientation();
	if (collinear(src, p1, p2)) {
		std::cerr << "geom: collinear " << src << " " << p1 << " " << p2 << std::endl;
		std::cerr << (p1 - src) << " " << (p2 - src) << std::endl;
		std::cerr << (p1 - src).cross(p2 - src) << std::endl;
		return std::make_pair(bestshot, 0);
	}
	std::vector<std::pair<double, int> > events;
	events.push_back(std::make_pair(0, 1)); // p1 becomes angle 0
	events.push_back(std::make_pair(angle_mod((p2 - src).orientation() - offangle), -1));
	for (size_t i = 0; i < obstacles.size(); ++i) {
		point diff = obstacles[i] - src;
		// warning: temporarily reduced
		if (diff.len() < radius) {
			std::cerr << "geom: inside" << std::endl;
			return std::make_pair(bestshot, 0);
		}
		const double cent = angle_mod(diff.orientation() - offangle);
		const double span = asin(radius / diff.len());
		const double range1 = cent - span;
		const double range2 = cent + span;
		// temporary fix
		if (range1 < -M_PI || range2 > M_PI) continue;
		events.push_back(std::make_pair(range1, -1));
		events.push_back(std::make_pair(range2, 1));
	}
	// do angle sweep for largest angle
	sort(events.begin(), events.end());
	double best = 0;
	double sum = 0;
	double start = events[0].first;
	int cnt = 0;
	for (size_t i = 0; i + 1 < events.size(); ++i) {
		cnt += events[i].second;
		assert(cnt <= 1);
		if (cnt > 0) {
			sum += events[i+1].first - events[i].first;
			if (best < sum) {
				best = sum;
				// shoot ray from point p
				// intersect with line p1-p2
				const double mid = start + sum / 2 + offangle;
				const point ray = point(cos(mid), sin(mid)) * 10.0;
				const point inter = line_intersect(src, src + ray, p1, p2);
				bestshot = inter;
			}
		} else {
			sum = 0;
			start = events[i+1].first;
		}
	}
	return std::make_pair(bestshot, best);
}

bool collinear(const point& a, const point& b, const point& c) {
	if ((a - b).lensq() < EPS2 || (b - c).lensq() < EPS2 || (a - c).lensq() < EPS2)
		return true;
	return (std::fabs((b - a).cross(c - a)) < EPS);
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

std::vector<point> line_circle_intersect(point centre, double radius, point segA, point segB){

  std::vector<point> ans;

  //take care of 0 length segments too much error here
  if((segB - segA).lensq()<EPS)return ans;

  double lenseg = (segB - segA).dot(centre-segA)/(segB-segA).len();
  point C = segA + lenseg*(segB-segA).norm();

  //if C outside circle no intersections
  if((C-centre).lensq() > radius*radius + EPS) return ans;
    
  //if C on circle perimeter return the only intersection
  if((C-centre).lensq() < radius*radius + EPS && (C-centre).lensq() > radius*radius - EPS){
    ans.push_back(C);
    return ans;
  }
  //first possible intersection
  double lensegb = radius*radius -(C-centre).lensq();
  point inter = C - lensegb*(segB-segA).norm();

  ans.push_back( C - lensegb*(segB-segA).norm());
  ans.push_back( C + lensegb*(segB-segA).norm());
 
  return ans;

}

std::vector<point> lineseg_circle_intersect(point centre, double radius, point segA, point segB){
  std::vector<point> ans;
  std::vector<point> poss = line_circle_intersect(centre, radius, segA, segB);

  for(unsigned int i =0; i<poss.size(); i++){
    bool x_ok = poss[i].x <= std::max(segA.x, segB.x)+EPS && poss[i].x >= std::min(segA.x, segB.x)-EPS; 
    bool y_ok = poss[i].y <= std::max(segA.y, segB.y)+EPS && poss[i].y >= std::min(segA.y, segB.y)-EPS;
    if(x_ok && y_ok)ans.push_back(poss[i]); 
  }
  return ans;
}

// ported code
point line_intersect(const point &a, const point &b, const point &c, const point &d) {
	assert((d - c).cross(b - a) != 0);
	return a + (a - c).cross(d - c) / (d - c).cross(b - a) * (b - a);
}

// ported code
double line_point_dist(const point &p, const point &a, const point &b) {
	return (p - a).cross(b - a) / (b - a).len();
}

// ported code
inline int sign(const double n) {
	return n > EPS ? 1 : (n < -EPS ? -1 : 0);
}

// ported code
bool seg_crosses_seg(const point& a1, const point& a2, const point &b1, const point &b2) {
	return sign((a2 - a1).cross(b1 - a1))
		* sign((a2 - a1).cross(b2 - a1)) <= 0 &&
		sign((b2 - b1).cross(a1 - b1))
		* sign((b2 - b1).cross(a2 - b1)) <= 0;
}

bool line_seg_intersect_rectangle(point seg[2], point recA[4]){
  bool intersect = point_in_rectangle(seg[0], recA) || point_in_rectangle(seg[1], recA);
  for(int i = 0; i<4; i++){
    for(int j=i+1; j<4; j++){
      intersect = intersect || seg_crosses_seg(seg[0], seg[1], recA[i], recA[j]);
    }
  }
  return intersect;
}

bool point_in_rectangle(point pointA, point recA[4]){

   bool x_ok = pointA.x >= std::min(std::min(recA[0].x,recA[1].x),std::min(recA[2].x, recA[3].x));
   x_ok = x_ok && pointA.x <= std::max(std::max(recA[0].x,recA[1].x),std::max(recA[2].x, recA[3].x)); 
  bool y_ok = pointA.y >= std::min(std::min(recA[0].y,recA[1].y),std::min(recA[2].y, recA[3].y));
  y_ok = y_ok && pointA.y <= std::max(std::max(recA[0].y,recA[1].y),std::max(recA[2].y, recA[3].y));

  // was this missing here??
  return x_ok && y_ok;
}

point reflect(const point& v, const point& n) {
	if (n.len() < 0) {
		std::cerr  << "geom: reflect: zero length" << std::endl;
		return v;
	}
	point normal = n / n.len();
	return 2 * v.dot(normal) * normal - v;
}

point reflect(const point& a, const point& b, const point& p) {
	// Make a as origin.
	// Rotate by 90 degrees, does not matter which direction?
	point n = (b - a).rotate(M_PI / 2.0);
	return a + reflect(p - a, n);
}
 
point calc_block_side_pos(const point& a, const point& b, const point& p, const double& radius, const double& thresh, const int side) {
#warning implement
	return (a + b) / 2.0;
}

// ported code
point calc_block_cone(const point &a, const point &b, const double& radius) {
	// unit vector and bisector
	point au = a / a.len();
	point c = au + b / b.len();
	// use similar triangle
	return c * (radius / std::fabs(au.cross(c)));
}

point calc_block_cone(const point &a, const point &b, const point& p, const double& radius) {
	point R = p + calc_block_cone(a - p, b - p, radius);
	#warning: THIS MAGIC THRESHOLD should be fixed after competition
	//TODO: Fix this magic number
	const double MIN_X = std::min(-2.5, (p.x + 3.025) / 2.0 - 3.025);
	if (R.x < MIN_X){
		R = (R - p) * ((MIN_X - p.x) / (R.x - p.x)) + p;
	}
	return R;
}

// ported code
point calc_block_other_ray(const point& a, const point& c, const point& g) {
	return reflect(a - c, g - c);
}

// ported code
bool goalie_block_goal_post(const point& a, const point& b, const point& c, const point& g) {
	point R = reflect(a - c, g - c);
	return (R.cross(b - c) < -EPS);
}

// ported code
point calc_block_cone_defender(const point& a, const point& b, const point& c, const point& g, const double& r) {
	point R = reflect(a - c, g - c);
	return calc_block_cone(R + c, b, c, r);
}

