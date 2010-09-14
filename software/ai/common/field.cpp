#include "ai/common/field.h"

using namespace AI::Common;

Point Field::friendly_goal() const {
	return Point(-length() * 0.5, 0);
}

Point Field::enemy_goal() const {
	return Point(length() * 0.5, 0);
}

Point Field::penalty_enemy() const {
	return Point(length() * 0.5 / 3.025 * (3.025 - 0.450), 0);
}

Point Field::penalty_friendly() const {
	return Point(-length() * 0.5 / 3.025 * (3.025 - 0.450), 0);
}

std::pair<Point, Point> Field::friendly_goal_boundary() const {
	return std::make_pair(Point(-length() * 0.5, -0.5 * goal_width()), Point(-length() * 0.5, 0.5 * goal_width()));
}

std::pair<Point, Point> Field::enemy_goal_boundary() const {
	return std::make_pair(Point(length() * 0.5, -0.5 * goal_width()), Point(length() * 0.5, 0.5 * goal_width()));
}

double Field::bounds_margin() const {
	return width() / 20.0;
}

