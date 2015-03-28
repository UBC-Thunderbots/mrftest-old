#include "geom/util.h"
#include "geom/seg.h"
#include "geom/line.h"

namespace Geom {
	constexpr Seg::Seg() : start(), end() { }

	Seg::Seg(const Point& start, const Point& end) : start(start), end(end) { }

	double Seg::slope() const {
		Point diff = start - end;
		return diff.y / diff.x;
	}

	bool Seg::degenerate() const {
		return (start - end).lensq() < Geom::EPS2;
	}

	Vector2 Seg::to_vector() const {
		return start - end;
	}

	Line Seg::to_line() const {
		return Line(start, end);
	}

	double Seg::len() const {
		return (start - end).len();
	}

	double Seg::lensq() const {
		return (start - end).lensq();
	}
}
