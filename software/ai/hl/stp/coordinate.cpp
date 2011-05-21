#include "ai/hl/stp/coordinate.h"
#include "util/dprint.h"

using AI::HL::STP::Coordinate;
using namespace AI::HL::STP;

Coordinate::Coordinate(const Point &pos) : world(NULL), y_type(YType::ABSOLUTE), o_type(OriginType::ABSOLUTE), pos(pos) {
}

Coordinate::Coordinate(const World &world, const Point &pos, YType y_type, OriginType o_type) : world(&world), y_type(y_type), o_type(o_type), pos(pos) {
}

Point Coordinate::operator()() const {
	Point p = pos;

	switch (o_type) {
		case OriginType::BALL:
			p += world->ball().position();
			break;

		case OriginType::ABSOLUTE:
		default:
			break;
	}

	switch (y_type) {
		case YType::BALL:
			if (world->ball().position().y < 0) {
				p.y *= -1;
			}
			break;

		case YType::MAJORITY:
			LOG_ERROR("NOT IMPLEMENTED YET");
			break;

		case YType::ABSOLUTE:
		default:
			break;
	}

	return pos;
}

