#include "ai/hl/stp/evaluate/coordinate.h"

using AI::HL::STP::Evaluate::Coordinate;
using namespace AI::HL::W;

Point Coordinate::CoordinateData::position() const {
	return Point(0.0, 0.0);
}

double Coordinate::CoordinateData::orientation() const {
	return 0;
}

Point Coordinate::evaluate() const {
	if (data.is()) {
		// TODO
		return data->position() + offset.rotate(data->orientation());
	} else {
		return offset;
	}
}

