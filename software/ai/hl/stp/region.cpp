#include "ai/hl/stp/region.h"
#include <cmath>

using AI::HL::STP::Region;

Region::Region(const Region &region) : type_(region.type_), p1(region.p1), p2(region.p2), radius_(region.radius_) {
}

Region::Region(Type type, Coordinate p1, Coordinate p2, double radius) : type_(type), p1(p1), p2(p2), radius_(radius) {
}

Region Region::circle(Coordinate center, double radius) {
	return Region(Type::CIRCLE, center, center, radius);
}

Region Region::rectangle(Coordinate p1, Coordinate p2) {
	return Region(Type::RECTANGLE, p1, p2, 0);
}

Point Region::center_position() const {
	if (type_ == Type::RECTANGLE) {
		return (p1.position() + p2.position()) / 2;
	} else {
		return p1.position();
	}
}

Point Region::center_velocity() const {
	if (type_ == Type::RECTANGLE) {
		return (p1.velocity() + p2.velocity()) / 2;
	} else {
		return p1.velocity();
	}
}

Point Region::random_sample() const {
	if (type_ ==  Type::RECTANGLE) {
		Point v0 = p1.position(), v1 = p2.position();
		double w = (std::rand() / static_cast<double>(RAND_MAX) * 2 * radius_) - radius_;
		double l = std::rand() / static_cast<double>(RAND_MAX) * (v0 - v1).len();

		return v0 + (v1 - v0).norm(l) + (v1 - v0).perp().norm(w);
	} else {
		double r = std::sqrt(std::rand() / static_cast<double>(RAND_MAX)) * radius_;
		double a = std::rand() / static_cast<double>(RAND_MAX) * 2 * M_PI;

		return p1.position() + Point(r, 0).rotate(a);
	}
}


bool Region::inside(Point p) const {
	if (type_ == Type::RECTANGLE) {
		return Rect(p1.position(), p2.position()).point_inside(p);
	} else {
		return (p - p1.position()).len() <= radius_;
	}
}

Region &Region::operator=(const Region &r) {
	type_ = r.type_;
	p1 = r.p1; 
	p2 = r.p2;
	radius_ = r.radius_;
}
