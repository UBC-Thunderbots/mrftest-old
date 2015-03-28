#include "geom/line.h"
#include "geom/util.h"

namespace Geom {
	constexpr Line::Line() : first(), second() { }

	Line::Line(const Point& first, const Point& second) : first(first), second(second) { }

	double Line::slope() const {
		Point diff = first - second;
		return diff.y / diff.x;
	}

	bool Line::degenerate() const {
		return (first - second).lensq() < Geom::EPS2;
	}
}
