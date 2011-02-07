#include "ai/hl/stp/coordinate.h"

using AI::HL::STP::Coordinate;
using namespace AI::HL::W;

Point Coordinate::CoordinateData::position() const {
	return Point(0.0, 0.0);
}

double Coordinate::CoordinateData::orientation() const {
	return 0;
}

Point Coordinate::operator()() const {
	if (data.is()) {
		// TODO
		return data->position() + pos.rotate(data->orientation());
	} else {
		return pos;
	}
}

