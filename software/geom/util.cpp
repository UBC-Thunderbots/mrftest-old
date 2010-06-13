#include "geom/util.h"

#include <cassert>
#include <algorithm>
#include <iostream>

namespace {
	const double EPS = 1e-9;
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
	if (collinear(src, p1, p2)) return std::make_pair(bestshot, 0);
	std::vector<std::pair<double, int> > events;
	events.push_back(std::make_pair((src - p1).orientation(), 1));
	events.push_back(std::make_pair((src - p2).orientation(), -1));
	for (size_t i = 0; i < obstacles.size(); ++i) {
		point diff = obstacles[i] - src;
		if (diff.len() < radius * 2.0) {
			return std::make_pair(bestshot, 0);
		}
		double cent = diff.orientation();
		double span = asin(radius / diff.len());
		// temporary fix
		if (cent - span < -M_PI || cent + span > M_PI) continue;
		events.push_back(std::make_pair(cent-span,-1));
		events.push_back(std::make_pair(cent+span,1));
	}
	// do angle sweep for largest angle
	sort(events.begin(), events.end());
	double best = 0;
	double sum = 0;
	double start = 0;
	double cnt = 0;
	for (size_t i = 0; i < events.size() - 1; ++i) {
		cnt += events[i].second;
		if (cnt > 0) {
			sum += events[i+1].first - events[i].first;
			if (best < sum) {
				best = sum;
				// shoot ray from point p
				// intersect with line p1-p2
				const double mid = start + sum / 2;
				const point ray = point(cos(mid), sin(mid));
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
	if ((a - b).lensq() < EPS || (b - c).lensq() < EPS || (a - c).lensq() < EPS)
		return true;
	return (abs((b - a).cross(c - a)) < EPS);
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
	return (R.cross(b - c) < -EPS);
}

// ported code
point defender_blocks_goal(const point& a, const point& b, const point& c, const point& g, const double& r) {
	point R = reflect(a - c, g- c);
	return calc_block_ray(R, b - c, r) + c;
}

