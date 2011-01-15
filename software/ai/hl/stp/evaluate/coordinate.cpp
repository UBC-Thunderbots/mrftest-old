#include "ai/hl/stp/evaluate/coordinate.h"

using AI::HL::STP::Evaluate::Coordinate;
using AI::HL::STP::Evaluate::CoordinateType;
using namespace AI::HL::W;

Coordinate::Coordinate(CoordinateType t, const Point& off) : type_(t), offset_(off) {
}

namespace {
	class Absolute : public Coordinate {
		private:
			const Point point;
		public:
			Absolute(Point p) : Coordinate(CoordinateType::COORDINATE_ABSOLUTE, p) {
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

