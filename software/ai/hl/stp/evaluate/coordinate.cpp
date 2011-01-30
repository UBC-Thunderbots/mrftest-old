#include "ai/hl/stp/evaluate/coordinate.h"

using AI::HL::STP::Evaluate::Coordinate;
using namespace AI::HL::W;

Coordinate::Coordinate(const Point& off) : offset_(off) {
}

namespace {
	class Absolute : public Coordinate {
		private:
			const Point point;
		public:
			Absolute(Point p) : Coordinate(p) {
			}
		private:
			Point evaluate() const {
				return offset_;
			}
			Coordinate::Ptr offset(const Point& off) {
				Coordinate::Ptr p(new Absolute(offset_ + off));
				return p;
			}
	};
}

Coordinate::Ptr Coordinate::absolute(const Point& point) {
	Coordinate::Ptr p(new Absolute(point));
	return p;
}

