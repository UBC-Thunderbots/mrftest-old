#include "ai/hl/stp/coordinate.h"
#include "util/dprint.h"

using AI::HL::STP::Coordinate;
using namespace AI::HL::STP;

Coordinate::Coordinate() : world(0), y_type(YType::ABSOLUTE), o_type(OriginType::ABSOLUTE) {
}

Coordinate::Coordinate(const Point &pos) : world(0), y_type(YType::ABSOLUTE), o_type(OriginType::ABSOLUTE), pos(pos) {
}

Coordinate::Coordinate(const Coordinate &coord) : world(coord.world), y_type(coord.y_type), o_type(coord.o_type), pos(coord.pos) {
}

Coordinate::Coordinate(const World &world, const Point &pos, YType y_type, OriginType o_type) : world(&world), y_type(y_type), o_type(o_type), pos(pos) {
}

Point Coordinate::position() const {
	Point p = pos;
	bool flip_y = false;
	double center = 0.0;

	switch (y_type) {
		case YType::BALL:
			if (world->ball().position().y < 0) {
				p.y *= -1;
				flip_y = true;
			}
			break;

		case YType::OUR_SIDE_STRONG:
			for (std::size_t i = 0; i < world->friendly_team().size(); i++) {
				center += world->friendly_team().get(i)->position().y;
			}
			if (center < 0.0) {
				p.y *= -1;
				flip_y = true;
			}
			break;

		case YType::THEIR_SIDE_STRONG:
			for (std::size_t i = 0; i < world->enemy_team().size(); i++) {
				center += world->enemy_team().get(i)->position().y;
			}
			if (center < 0.0) {
				p.y *= -1;
				flip_y = true;
			}
			break;

		case YType::ABSOLUTE:
		default:
			break;
	}

	switch (o_type) {
		case OriginType::BALL:
			p.x += world->ball().position().x;
			p.y += world->ball().position().y;
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

Coordinate &Coordinate::operator=(const Coordinate &c) {
	y_type = c.y_type;
	o_type = c.o_type;
	pos = c.pos;
	return *this;
}

