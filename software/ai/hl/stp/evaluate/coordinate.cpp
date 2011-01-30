#include "ai/hl/stp/evaluate/coordinate.h"

using AI::HL::STP::Evaluate::Coordinate;
using namespace AI::HL::W;

Point Coordinate::CoordinateData::position() const {
	return Point(0.0, 0.0);
}

double Coordinate::CoordinateData::orientation() const {
	return 0;
}

Coordinate::Coordinate() : offset(0.0, 0.0) {
}

Coordinate::Coordinate(const Point& off) : offset(off) {
}

Coordinate::Coordinate(const Coordinate& coordinate) {
	offset = coordinate.offset;
	data = coordinate.data;
}

Point Coordinate::evaluate() const {
	if (data.is()) {
		// TODO
		return data->position() + offset.rotate(data->orientation());
	} else {
		return offset;
	}
}

