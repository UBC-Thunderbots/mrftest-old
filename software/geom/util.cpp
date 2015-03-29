#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/hungarian.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

namespace Geom {
	double dist(const Vector2& first, const Vector2& second) {
		return (first - second).len();
	}
	double dist(const Line& first, const Vector2& second) {
		if (is_degenerate(first)) {
			return dist(first.first, second);
		}
		return fabs((second - first.first).cross(first.second - first.first)
			/ (first.second - first.first).len());
	}

	double dist(const Vector2& first, const Line& second) {
		return dist(second, first);
	}

	double dist(const Vector2& first, const Seg& second) {
		return proj_dist(second.start, second.end, first) > 0 
			&& proj_dist(second.end, second.start, first) > 0
			? std::fabs(dist(as_line(second), first)) 
			: std::min(dist(second.start, first), dist(second.end, first));
	}
	double dist(const Seg& first, const Vector2& second) {
		return dist(second, first);
	}

	double distsq(const Vector2& first, const Vector2& second) {
		return (first - second).lensq();
	}

	double slope(const Seg& seg) {
		Vector2 diff = seg.start - seg.end;
		return diff.y / diff.x;
	}
	double slope(const Line& line) {
		Vector2 diff = line.first - line.second;
		return diff.y / diff.x;
	}

	bool is_degenerate(const Seg& seg) {
		return distsq(seg.start, seg.end) < EPS2;
	}
	
	bool is_degenerate(const Line& line) {
		return distsq(line.first, line.second) < EPS2;
	}

	Vector2 as_vector2(const Seg& seg) {
		return seg.end - seg.start;
	}

	Line as_line(const Seg& seg) {
		return Line(seg.start, seg.end);
	}

	double len(const Seg& seg) {
		return dist(seg.start, seg.end);
	}

	double len(const Line& line) {
		(void) line; // unused
		return std::numeric_limits<double>::infinity();
	}

	double lensq(const Seg& seg) {
		return distsq(seg.start, seg.end);
	}

	double lensq(const Line& line) {
		(void) line; // unused
		return std::numeric_limits<double>::infinity();
	}

	bool contains(const Triangle& out, const Vector2& in) {
		double angle = 0;
		for (int i = 0, j = 2; i < 3; j = i++) {
			if ((in - out[i]).len() < EPS) {
				return true; // SPECIAL CASE
			}
			double a = atan2((out[i] - in).cross(out[j] - in),
					(out[i] - in).dot(out[j] - in));
			angle += a;
		}
		return std::fabs(angle) > 6;
	}

	bool contains(const Circle& out, const Vector2& in) {
		return distsq(out.origin, in) <= out.radius * out.radius;
	}

	bool contains(const Circle& out, const Seg& in) {
		return dist(in, out.origin) < out.radius;
	}

	bool intersects(const Triangle &first, const Circle& second) {
		return contains(first, second.origin) 
			|| dist(get_side<3>(first, 0), second.origin) < second.radius
			|| dist(get_side<3>(first, 1), second.origin) < second.radius
			|| dist(get_side<3>(first, 2), second.origin) < second.radius;
	}
	bool intersects(const Circle &first, const Triangle& second) {
		return intersects(second, first);
	}

	bool intersects(const Seg &first, const Circle &second) {
		// if the segment is inside the circle AND at least one of the points is outside the circle
		return contains(second, first) 
			&& (distsq(first.start, second.origin) > second.radius * second.radius
				|| distsq(first.end, second.origin) > second.radius * second.radius);
	}
	bool intersects(const Circle &first, const Seg &second) {
		return intersects(second, first);
	}

	bool intersects(const Seg& first, const Seg& second) {
		if (sign((first.start - first.end).cross(second.start - second.end)) == 0) {
			// find distance of two endpoints on segments furthest away from each other
			double mx_len = std::max(std::max((second.start - first.end).len(), (second.end - first.end).len()), std::max((second.start - first.start).len(), (second.end - first.start).len()));
			// if the segments cross then this distance should be less than
			// the sum of the distances of the line segments
			return mx_len < (first.start - first.end).len() + (second.start - second.end).len() + EPS;
		}

	return sign((first.end - first.start).cross(second.start - first.start)) * sign((first.end - first.start).cross(second.end - first.start)) <= 0 && sign((second.end - second.start).cross(first.start - second.start)) * sign((second.end - second.start).cross(first.end - second.start)) <= 0;
	}

	template<unsigned int N>
	Vector2 get_vertex(const std::array<Vector2, N>& poly, unsigned int i) {
		return poly[i % N];
	}

	template<unsigned int N>
	void set_vertex(std::array<Vector2, N>& poly, unsigned int i, const Vector2& v) {
		poly[i % N] = v;
	}

	template<unsigned int N>
	Seg get_side(const std::array<Vector2, N>& poly, unsigned int i) {
		return Seg(get_vertex<N>(poly, i), get_vertex<N>(poly, i + 1));
	}
}

using namespace Geom;

double proj_dist(const Point A, const Point B, const Point P) {
	return (B - A).dot(P - A) / (B - A).len();
}

double seg_pt_dist(const Point a, const Point b, const Point p) {
	return dist(Seg(a, b), p);
}

std::vector<std::size_t> dist_matching(const std::vector<Point> &v1, const std::vector<Point> &v2) {
	if (v1.size() != v2.size())
		LOG_ERROR(u8"vector sizes not equal");

	if (v1.size() > 1) {
		//use hungarian O(n^3)
		Hungarian hung(v1.size());
		for(std::size_t i = 0; i < v1.size(); i++)
			for(std::size_t o = 0; o < v1.size(); o++)
				hung.weight(i, o) = 0 - (v2[i] - v1[o]).len(); 	//this is negative because hungarian tries to maximize the total weight
										//use lensq instead to put more weight on outliers?

		hung.execute();
		std::vector<std::size_t> order(v1.size());

		for(std::size_t i = 0; i < v1.size(); i++)
			order[i] = hung.matchX(i);

		return order;
	}
	else {
		//use permutative O(n!)
		std::vector<std::size_t> order(v2.size());
		for (std::size_t i = 0; i < v2.size(); ++i) {
			order[i] = i;
		}
		std::vector<std::size_t> best = order;
		double bestscore = 1e99;
		const std::size_t N = std::min(v1.size(), v2.size());
		do {
			double score = 0;
			for (std::size_t i = 0; i < N; ++i) {
				score += (v1[i] - v2[order[i]]).len();
			}
			if (score < bestscore) {
				bestscore = score;
				best = order;
			}
		} while (std::next_permutation(order.begin(), order.end()));
		return best;
	}
}

bool point_in_triangle(const Point p1, const Point p2, const Point p3, const Point p) {
	return contains(Triangle({ p1, p2, p3 }), p);
}

bool triangle_circle_intersect(const Point p1, const Point p2, const Point p3, const Point c, const double radius) {
	return intersects(Triangle({ p1, p2, p3 }), Circle(c, radius));
}

std::vector<std::pair<Point, Angle>> angle_sweep_circles_all(const Point &src, const Point &p1, const Point &p2, const std::vector<Point> &obstacles, const double &radius) {
	std::vector<std::pair<Point, Angle>> ret;

	const Angle offangle = (p1 - src).orientation();
	if (collinear(src, p1, p2)) {
		// std::cerr << "geom: collinear " << src << " " << p1 << " " << p2 << std::endl;
		// std::cerr << (p1 - src) << " " << (p2 - src) << std::endl;
		// std::cerr << (p1 - src).cross(p2 - src) << std::endl;
		return ret;
	}

	std::vector<std::pair<Angle, int>> events;
	events.reserve(2 * obstacles.size() + 2);
	events.push_back(std::make_pair(Angle::zero(), 1)); // p1 becomes angle 0
	events.push_back(std::make_pair(((p2 - src).orientation() - offangle).angle_mod(), -1));
	for (Point i : obstacles) {
		Point diff = i - src;
		// warning: temporarily reduced
		if (diff.len() < radius) {
			// std::cerr << "geom: inside" << std::endl;
			return ret;
		}
		const Angle cent = (diff.orientation() - offangle).angle_mod();
		const Angle span = Angle::asin(radius / diff.len());
		const Angle range1 = cent - span;
		const Angle range2 = cent + span;

		// "warning: hack should work" was removed since both this and the next function are functioning properly
		if (range1 < -Angle::half() || range2 > Angle::half()) {
			continue;
		}
		events.push_back(std::make_pair(range1, -1));
		events.push_back(std::make_pair(range2, 1));
	}
	// do angle sweep for largest angle
	std::sort(events.begin(), events.end());
	Angle sum = Angle::zero();
	Angle start = events[0].first;
	int cnt = 0;
	for (std::size_t i = 0; i + 1 < events.size(); ++i) {
		cnt += events[i].second;
		assert(cnt <= 1);
		if (cnt > 0) {
			sum += events[i + 1].first - events[i].first;
		} else {
			const Angle mid = start + sum / 2 + offangle;
			const Point ray = Point::of_angle(mid) * 10.0;
			const Point inter = line_intersect(src, src + ray, p1, p2);

			ret.push_back(std::make_pair(inter, sum));

			sum = Angle::zero();
			start = events[i + 1].first;
		}
	}
	return ret;
}

std::pair<Point, Angle> angle_sweep_circles(const Point &src, const Point &p1, const Point &p2, const std::vector<Point> &obstacles, const double &radius) {
	// default value to return if nothing is valid
	Point bestshot = (p1 + p2) * 0.5;
	const Angle offangle = (p1 - src).orientation();
	if (collinear(src, p1, p2)) {
		// std::cerr << "geom: collinear " << src << " " << p1 << " " << p2 << std::endl;
		// std::cerr << (p1 - src) << " " << (p2 - src) << std::endl;
		// std::cerr << (p1 - src).cross(p2 - src) << std::endl;
		return std::make_pair(bestshot, Angle::zero());
	}
	std::vector<std::pair<Angle, int>> events;
	events.reserve(2 * obstacles.size() + 2);
	events.push_back(std::make_pair(Angle::zero(), 1)); // p1 becomes angle 0
	events.push_back(std::make_pair(((p2 - src).orientation() - offangle).angle_mod(), -1));
	for (Point i : obstacles) {
		Point diff = i - src;
		// warning: temporarily reduced
		if (diff.len() < radius) {
			// std::cerr << "geom: inside" << std::endl;
			return std::make_pair(bestshot, Angle::zero());
		}
		const Angle cent = (diff.orientation() - offangle).angle_mod();
		const Angle span = Angle::asin(radius / diff.len());
		const Angle range1 = cent - span;
		const Angle range2 = cent + span;

		/*
		   if (range1 < -M_PI) {
		   // [-PI, range2]
		   events.push_back(std::make_pair(-M_PI, -1));
		   events.push_back(std::make_pair(range2, 1));
		   // [range1, PI]
		   events.push_back(std::make_pair(range1 + 2 * M_PI, -1));
		   events.push_back(std::make_pair(M_PI, 1));
		   } else if (range2 > M_PI) {
		   // [range1, PI]
		   events.push_back(std::make_pair(range1, -1));
		   events.push_back(std::make_pair(M_PI, 1));
		   // [-PI, range2]
		   events.push_back(std::make_pair(-M_PI, -1));
		   events.push_back(std::make_pair(range2 - 2 * M_PI, 1));
		   } else {
		   events.push_back(std::make_pair(range1, -1));
		   events.push_back(std::make_pair(range2, 1));
		   }
		 */

		if (range1 < -Angle::half() || range2 > Angle::half()) {
			continue;
		}
		events.push_back(std::make_pair(range1, -1));
		events.push_back(std::make_pair(range2, 1));
	}
	// do angle sweep for largest angle
	std::sort(events.begin(), events.end());
	Angle best = Angle::zero();
	Angle sum = Angle::zero();
	Angle start = events[0].first;
	int cnt = 0;
	for (std::size_t i = 0; i + 1 < events.size(); ++i) {
		cnt += events[i].second;
		assert(cnt <= 1);
		if (cnt > 0) {
			sum += events[i + 1].first - events[i].first;
			if (best < sum) {
				best = sum;
				// shoot ray from point p
				// intersect with line p1-p2
				const Angle mid = start + sum / 2 + offangle;
				const Point ray = Point::of_angle(mid) * 10.0;
				const Point inter = line_intersect(src, src + ray, p1, p2);
				bestshot = inter;
			}
		} else {
			sum = Angle::zero();
			start = events[i + 1].first;
		}
	}
	return std::make_pair(bestshot, best);
}

std::vector<Point> seg_buffer_boundaries(const Point &a, const Point &b, double buffer, int num_points) {
	if ((a - b).lensq() < EPS) {
		return circle_boundaries(a, buffer, num_points);
	}
	std::vector<Point> ans;

	double line_seg = (a - b).len();
	double semi_circle = M_PI * buffer;
	double total_dist = 2 * line_seg + 2 * semi_circle;
	double total_travelled = 0.0;
	double step_len = total_dist / num_points;
	Point add1(0.0, 0.0);
	Point add2 = buffer * ((a - b)).rotate(Angle::quarter()).norm();
	Point seg_direction = (b - a).norm();
	bool swapped = false;

	for (int i = 0; i < num_points; i++) {
		Point p = a + add1 + add2;
		ans.push_back(p);
		double travel_left = step_len;

		if (total_travelled < line_seg) {
			double l_travel = std::min(travel_left, line_seg - total_travelled);
			add1 += l_travel * seg_direction;
			travel_left -= l_travel;
			total_travelled += l_travel;
		}

		if (travel_left < EPS) {
			continue;
		}

		if (total_travelled + EPS >= line_seg && total_travelled < line_seg + semi_circle) {
			double l_travel = std::min(travel_left, line_seg + semi_circle - total_travelled);
			Angle rads = Angle::of_radians(l_travel / buffer);
			add2 = add2.rotate(rads);
			travel_left -= l_travel;
			total_travelled += l_travel;
		}

		if (travel_left < EPS) {
			continue;
		}

		if (total_travelled + EPS >= line_seg + semi_circle && total_travelled < 2 * line_seg + semi_circle) {
			if (!swapped) {
				seg_direction = -seg_direction;
				swapped = true;
			}
			double l_travel = std::min(travel_left, 2 * line_seg + semi_circle - total_travelled);
			add1 += l_travel * seg_direction;
			travel_left -= l_travel;
			total_travelled += l_travel;
		}

		if (travel_left < EPS) {
			continue;
		}

		if (total_travelled + EPS >= 2 * line_seg) {
			Angle rads = Angle::of_radians(travel_left / buffer);
			add2 = add2.rotate(rads);
			total_travelled += travel_left;
		}
	}
	return ans;
}

std::vector<Point> circle_boundaries(const Point &centre, double radius, int num_points) {
	Angle rotate_amount = Angle::full() / num_points;
	std::vector<Point> ans;
	Point bound(radius, 0.0);
	for (int i = 0; i < num_points; i++) {
		Point temp = centre + bound;
		ans.push_back(temp);
		bound = bound.rotate(rotate_amount);
	}
	return ans;
}

bool collinear(const Point &a, const Point &b, const Point &c) {
	if ((a - b).lensq() < EPS2 || (b - c).lensq() < EPS2 || (a - c).lensq() < EPS2) {
		return true;
	}
	return std::fabs((b - a).cross(c - a)) < EPS;
}

Point clip_point(const Point &p, const Point &bound1, const Point &bound2) {
	const double minx = std::min(bound1.x, bound2.x);
	const double miny = std::min(bound1.y, bound2.y);
	const double maxx = std::max(bound1.x, bound2.x);
	const double maxy = std::max(bound1.y, bound2.y);
	Point ret = p;
	if (p.x < minx) {
		ret.x = minx;
	} else if (p.x > maxx) {
		ret.x = maxx;
	}
	if (p.y < miny) {
		ret.y = miny;
	} else if (p.y > maxy) {
		ret.y = maxy;
	}
	return ret;
}

Point clip_point(const Point &p, const Rect &r) {
	const double minx = r.sw_corner().x;
	const double miny = r.sw_corner().y;
	const double maxx = r.ne_corner().x;
	const double maxy = r.ne_corner().y;
	Point ret = p;
	if (p.x < minx) {
		ret.x = minx;
	} else if (p.x > maxx) {
		ret.x = maxx;
	}
	if (p.y < miny) {
		ret.y = miny;
	} else if (p.y > maxy) {
		ret.y = maxy;
	}
	return ret;
}

std::vector<Point> line_circle_intersect(const Point &centre, double radius, const Point &segA, const Point &segB) {
	std::vector<Point> ans;

	// take care of 0 length segments too much error here
	if ((segB - segA).lensq() < EPS) {
		return ans;
	}

	double lenseg = (segB - segA).dot(centre - segA) / (segB - segA).len();
	Point C = segA + lenseg * (segB - segA).norm();

	// if C outside circle no intersections
	if ((C - centre).lensq() > radius * radius + EPS) {
		return ans;
	}

	// if C on circle perimeter return the only intersection
	if ((C - centre).lensq() < radius * radius + EPS && (C - centre).lensq() > radius * radius - EPS) {
		ans.push_back(C);
		return ans;
	}
	// first possible intersection
	double lensegb = radius * radius - (C - centre).lensq();

	ans.push_back(C - lensegb * (segB - segA).norm());
	ans.push_back(C + lensegb * (segB - segA).norm());

	return ans;
}

bool seg_intersects_circle(const Point& a, const Point& b, const Point& c, double r) {
	return intersects(Seg(a, b), Circle(c, r));
}

bool seg_inside_circle(const Point& a, const Point& b, const Point& c, double r) {
	return contains(Circle(c, r), Seg(a, b));
}


std::vector<Point> line_rect_intersect(const Rect &r, const Point &segA, const Point &segB) {
	std::vector<Point> ans;
	for (unsigned int i = 0; i < 4; i++) {
		const Point &a = r[i];
		const Point &b = r[i + 1];
		if (seg_crosses_seg(a, b, segA, segB) && unique_line_intersect(a, b, segA, segB)) {
			ans.push_back(line_intersect(a, b, segA, segB));
		}
	}
	return ans;
}

namespace {
	bool point_in_seg(const Point &p, const Point &segA, const Point &segB) {
		if (collinear(p, segA, segB)) {
			if ((p.x <= segA.x && p.x >= segB.x) || (p.x <= segB.x && p.x >= segA.x)) {
				// if( (p.y<= segA.y && p.y >= segB.y) || (p.y <= segB.y && p.y >= segA.y ) ){
				return true;
				/*} else {
					return false;
				   }*/
			} else {
				return false;
			}
		} else {
			LOG_ERROR(u8"not collinear");
			return false;
		}
	}

	bool point_in_vec(const Point &p, const Point &vecA, const Point &vecB) { // vecA is the beginning of the vector
		if (collinear(p, vecA, vecB)) {
			if (((p.x - vecA.x) * (vecB.x - vecA.x) > EPS) && ((p.y - vecA.y) * (vecB.y - vecA.y) > EPS)) {
				return true;
			} else {
				return false;
			}
		} else {
			LOG_ERROR(u8"not collinear");
			return false;
		}
	}
}



Point vector_rect_intersect(const Rect &r, const Point &vecA, const Point &vecB) {
	/*std::cout << vecA << vecB << r.ne_corner() << r.sw_corner();
	   for (unsigned int i = 0; i < 4; i++) {
	    unsigned int j = (i + 1)%4;
	    const Point &a = r[i];
	    const Point &b = r[j];
	    if ( vector_crosses_seg(vecA, vecB, a, b ) ) {
	        Point intersect = line_intersect(a, b, vecA, vecB);
	        std::cout << std::endl;
	        return intersect;
	    }
	   }
	   std::cout << "fail \n";
	   return r.centre();  // return the center of the rectangle, if no valid answer is found*/
	std::vector<Point> points = line_rect_intersect(r, vecA, (vecB - vecA) * 100 + vecA);
	for (Point i : points) {
		if (point_in_vec(i, vecA, vecB)) {
			return i;
		}
	}
	return Point(1.0/0.0, 1.0/0.0); //no solution found, propagate infinity
}


double lineseg_point_dist(const Point &centre, const Point &segA, const Point &segB) {
	// if one of the end-points is extremely close to the centre point
	// then return 0.0
	if ((segB - centre).lensq() < EPS2 || (segA - centre).lensq() < EPS2) {
		return 0.0;
	}

	// take care of 0 length segments
	if ((segB - segA).lensq() < EPS2) {
		return std::min((centre - segB).len(), (centre - segA).len());
	}

	// find point C
	// which is the projection onto the line
	double lenseg = (segB - segA).dot(centre - segA) / (segB - segA).len();
	Point C = segA + lenseg * (segB - segA).norm();

	// check if C is in the line seg range
	double AC = (segA - C).lensq();
	double BC = (segB - C).lensq();
	double AB = (segA - segB).lensq();
	bool in_range = AC <= AB && BC <= AB;

	// if so return C
	if (in_range) {
		if ((centre - C).lensq() < EPS2) {
			return 0.0;
		}
		return (centre - C).len();
	}
	// otherwise return distance to closest end of line-seg
	return std::min((centre - segB).len(), (centre - segA).len());
}

Point closest_lineseg_point(const Point &centre, const Point &segA, const Point &segB) {
	// if one of the end-points is extremely close to the centre point
	// then return 0.0
	if ((segB - centre).lensq() < EPS2) {
		return segB;
	}

	if ((segA - centre).lensq() < EPS2) {
		return segA;
	}

	// take care of 0 length segments
	if ((segB - segA).lensq() < EPS2) {
		return segA;
	}

	// find point C
	// which is the projection onto the line
	double lenseg = (segB - segA).dot(centre - segA) / (segB - segA).len();
	Point C = segA + lenseg * (segB - segA).norm();

	// check if C is in the line seg range
	double AC = (segA - C).lensq();
	double BC = (segB - C).lensq();
	double AB = (segA - segB).lensq();
	bool in_range = AC <= AB && BC <= AB;

	// if so return C
	if (in_range) {
		return C;
	}
	double lenA = (centre - segA).len();
	double lenB = (centre - segB).len();

	// otherwise return closest end of line-seg
	if (lenA < lenB) {
		return segA;
	}
	return segB;
}

double seg_seg_distance(const Point &a, const Point &b, const Point &c, const Point &d) {
	if (seg_crosses_seg(a, b, c, d)) {
		return 0.0;
	}
	return std::min(std::min(lineseg_point_dist(a, c, d), lineseg_point_dist(b, c, d)), std::min(lineseg_point_dist(c, a, b), lineseg_point_dist(d, a, b)));
}

namespace {
//this function is never used
	std::vector<Point> lineseg_circle_intersect(const Point &centre, double radius, const Point &segA, const Point &segB) {
		std::vector<Point> ans;
		std::vector<Point> poss = line_circle_intersect(centre, radius, segA, segB);

		for (Point i : poss) {
			bool x_ok = i.x <= std::max(segA.x, segB.x) + EPS && i.x >= std::min(segA.x, segB.x) - EPS;
			bool y_ok = i.y <= std::max(segA.y, segB.y) + EPS && i.y >= std::min(segA.y, segB.y) - EPS;
			if (x_ok && y_ok) {
				ans.push_back(i);
			}
		}
		return ans;
	}
}

bool unique_line_intersect(const Point &a, const Point &b, const Point &c, const Point &d) {
	return std::abs((d - c).cross(b - a)) > EPS;
}

// ported code
Point line_intersect(const Point &a, const Point &b, const Point &c, const Point &d) {
	assert(std::abs((d - c).cross(b - a)) > EPS);
	return a + (a - c).cross(d - c) / (d - c).cross(b - a) * (b - a);
}

// ported code
bool seg_crosses_seg(const Point &a1, const Point &a2, const Point &b1, const Point &b2) {
	return intersects(Seg(a1, a2), Seg(b1, b2));
}

bool vector_crosses_seg(const Point &a1, const Point &a2, const Point &b1, const Point &b2) {
	if (std::abs((a1 - a2).cross(b1 - b2)) > EPS) {
		Point i0 = line_intersect(a1, a2, b1, b2);
		if (point_in_vec(i0, a1, a2) && point_in_seg(i0, b1, b2)) {
			return true;
		} else {
			return false;
		}
	} else {
		if (collinear(a1, a2, b1)) {
			return true;
		} else {
			return false;
		}
	}
}


bool line_seg_intersect_rectangle(const Point seg[2], const Point recA[4]) {
	bool intersect = point_in_rectangle(seg[0], recA) || point_in_rectangle(seg[1], recA);
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 4; j++) {
			intersect = intersect || seg_crosses_seg(seg[0], seg[1], recA[i], recA[j]);
		}
	}
	return intersect;
}

bool point_in_rectangle(const Point &pointA, const Point recA[4]) {
	/*
	bool x_ok = pointA.x >= std::min(std::min(recA[0].x, recA[1].x), std::min(recA[2].x, recA[3].x));
	x_ok = x_ok && pointA.x <= std::max(std::max(recA[0].x, recA[1].x), std::max(recA[2].x, recA[3].x));
	bool y_ok = pointA.y >= std::min(std::min(recA[0].y, recA[1].y), std::min(recA[2].y, recA[3].y));
	y_ok = y_ok && pointA.y <= std::max(std::max(recA[0].y, recA[1].y), std::max(recA[2].y, recA[3].y));
	return x_ok && y_ok;
	*/
	Point rectA = Point(std::min(std::min(recA[0].x, recA[1].x), std::min(recA[2].x, recA[3].x)),
				std::min(std::min(recA[0].y, recA[1].y), std::min(recA[2].y, recA[3].y)));
	Point rectB = Point(std::max(std::max(recA[0].x, recA[1].x), std::max(recA[2].x, recA[3].x)),
				std::max(std::max(recA[0].y, recA[1].y), std::max(recA[2].y, recA[3].y)));

	return point_in_rectangle(pointA, rectA, rectB);
}

bool point_in_rectangle(const Point &pointA, const Point &cornerA, const Point &cornerB) {
	return pointA.x >= std::min(cornerA.x, cornerB.x) && pointA.x <= std::max(cornerA.x, cornerB.x) &&
		pointA.y >= std::min(cornerA.y, cornerB.y) && pointA.y <= std::max(cornerA.y, cornerB.y);
}


Point reflect(const Point &v, const Point &n) {
	if (n.len() < EPS) {
		LOG_ERROR(u8"zero length");
		return v;
	}
	Point normal = n.norm();
	return v - 2 * v.dot(normal) * normal;
}

Point reflect(const Point &a, const Point &b, const Point &p) {
	// Make a as origin.
	// Rotate by 90 degrees, does not matter which direction?
	Point n = (b - a).rotate(Angle::quarter());
	return a + reflect(p - a, n);
}

// ported code
Point calc_block_cone(const Point &a, const Point &b, const double &radius) {
	if (a.len() < EPS || b.len() < EPS) {
		LOG_ERROR(u8"block cone zero vectors");
	}
	// unit vector and bisector
	Point au = a / a.len();
	Point c = au + b / b.len();
	// use similar triangle
	return c * (radius / std::fabs(au.cross(c)));
}

Point calc_block_cone(const Point &a, const Point &b, const Point &p, const double &radius) {
	/*
	   Point R = p + calc_block_cone(a - p, b - p, radius);
	   const double MIN_X = std::min(-2.5, (p.x + 3.025) / 2.0 - 3.025);
	   if (R.x < MIN_X){
	   R = (R - p) * ((MIN_X - p.x) / (R.x - p.x)) + p;
	   }
	   return R;
	 */
	return p + calc_block_cone(a - p, b - p, radius);
}

// ported code
Point calc_block_other_ray(const Point &a, const Point &c, const Point &g) {
	return reflect(c - a, g - c); //this, and the next two instances, were changed from a - c since reflect() was fixed 
}

// ported code
bool goalie_block_goal_post(const Point &a, const Point &b, const Point &c, const Point &g) {
	Point R = reflect(c - a, g - c);
	return fabs(R.cross(b - c)) < EPS; 
}

// ported code
Point calc_block_cone_defender(const Point &a, const Point &b, const Point &c, const Point &g, const double &r) {
	Point R = reflect(c - a, g - c); 
	// std::cout << (R + c) << std::endl;
	return calc_block_cone(R + c, b, c, r);
}

// ported cm code below

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

Angle vertex_angle(Point a, Point b, Point c) {
	return ((a - b).orientation() - (c - b).orientation()).angle_mod();
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

bool point_in_front_vector(Point offset, Point dir, Point p) {
	// compare angle different
	Angle a1 = dir.orientation();
	Angle a2 = (p - offset).orientation();
	Angle diff = (a1 - a2).angle_mod();
	return diff < Angle::quarter() && diff > -Angle::quarter();
}

