#include "ai/hl/stp/region.h"

Point AI::HL::STP::Rectangle::center() const {
	return (p1() + p2()) / 2;
}

Rect AI::HL::STP::Rectangle::evaluate() const {
	return Rect(p1(), p2());
}

bool AI::HL::STP::Rectangle::inside(Point p) const {
	return Rect(p1(), p2()).point_inside(p);
}

bool AI::HL::STP::Circle::inside(Point p) const {
	return (p - center_()).len() <= radius_;
}

