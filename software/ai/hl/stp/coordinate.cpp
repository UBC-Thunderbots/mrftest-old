#include "ai/hl/stp/coordinate.h"
#include "util/dprint.h"

using AI::HL::STP::Coordinate;
using namespace AI::HL::STP;

Coordinate::Coordinate() : world(), y_type(YType::ABSOLUTE), o_type(OriginType::ABSOLUTE) {
}

Coordinate::Coordinate(Point pos) : world(), y_type(YType::ABSOLUTE), o_type(OriginType::ABSOLUTE), pos(pos) {
}

Coordinate::Coordinate(const Coordinate &coord) : world(coord.world.get() ? new World(*coord.world.get()) : nullptr), y_type(coord.y_type), o_type(coord.o_type), pos(coord.pos) {
}

Coordinate::Coordinate(World world, Point pos, YType y_type, OriginType o_type) : world(new World(world)), y_type(y_type), o_type(o_type), pos(pos) {
}

Point Coordinate::position() const {
	Point p = pos;
	double center = 0.0;

	switch (y_type) {
		case YType::BALL:
			if (world->ball().position().y < 0) {
				p.y *= -1;
			}
			break;

		case YType::OUR_SIDE_STRONG:
			for (const Player i : world->friendly_team()) {
				center += i.position().y;
			}
			if (center < 0.0) {
				p.y *= -1;
			}
			break;

		case YType::THEIR_SIDE_STRONG:
			for (const Robot i : world->enemy_team()) {
				center += i.position().y;
			}
			if (center < 0.0) {
				p.y *= -1;
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

bool Coordinate::operator==(const Coordinate &other) const {
	if (!(y_type == other.y_type && o_type == other.o_type && pos == other.pos)) {
		return false;
	}
	if (world) {
		return other.world && (*world.get() == *other.world.get());
	} else {
		return !other.world;
	}
}

std::size_t Coordinate::hash() const {
	std::size_t acc = 5;
	acc = acc * 17 + std::hash<unsigned int>()(static_cast<unsigned int>(y_type));
	acc = acc * 17 + std::hash<unsigned int>()(static_cast<unsigned int>(o_type));
	acc = acc * 17 + std::hash<Point>()(pos);
	return acc;
}

