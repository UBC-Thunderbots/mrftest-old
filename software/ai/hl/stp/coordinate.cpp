#include "ai/hl/stp/coordinate.h"
#include "util/dprint.h"

using AI::HL::STP::Coordinate;
using namespace AI::HL::STP;

Coordinate::Coordinate(const Point &pos) : world(0), y_type(YType::ABSOLUTE), o_type(OriginType::ABSOLUTE), pos(pos) {
}

Coordinate::Coordinate(const Coordinate &coord) : world(coord.world), y_type(coord.y_type), o_type(coord.o_type), pos(coord.pos) {
}

Coordinate::Coordinate(const World &world, const Point &pos, YType y_type, OriginType o_type) : world(&world), y_type(y_type), o_type(o_type), pos(pos) {
}

Point Coordinate::position() const {
	Point p = pos;
	bool flip_y = false;

	switch (y_type) {
		case YType::BALL:
			if (world->ball().position().y < 0) {
				p.y *= -1;
				flip_y = true;
			}
			break;

		case YType::OUR_SIDE_STRONG:
			LOG_ERROR("NOT IMPLEMENTED YET");
			break;

		case YType::THEIR_SIDE_STRONG:
			LOG_ERROR("NOT IMPLEMENTED YET");
			break;

		case YType::ABSOLUTE:
		default:
			break;
	}

	switch (o_type) {
		case OriginType::BALL:
			p.x += world->ball().position().x;
			if (flip_y) {
				p.y -= world->ball().position().y;
			} else {
				p.y += world->ball().position().y;
			}
			break;

		case OriginType::ABSOLUTE:
		default:
			break;
	}

	return p;
}

Point Coordinate::velocity() const {
	if (o_type == OriginType::BALL) {
		return world->ball().velocity();
	}
	return Point();
}

