#include "ai/common/field.h"

using namespace AI::Common;

Point Field::friendly_goal() const {
	return Point(-length() * 0.5, 0);
}

Point Field::enemy_goal() const {
	return Point(length() * 0.5, 0);
}

Point Field::penalty_enemy() const {
	return Point(length() * 0.5 / 3.025 * (3.025 - 0.750), 0);
}

Point Field::penalty_friendly() const {
	return Point(-length() * 0.5 / 3.025 * (3.025 - 0.750), 0);
}

Point Field::friendly_corner_pos() const {
	return Point(friendly_goal().x, length() / 2);
}

Point Field::friendly_corner_neg() const {
	return Point(friendly_goal().x, - length() / 2);
}

Point Field::enemy_corner_pos() const {
	return Point(enemy_goal().x, length() / 2);
}

Point Field::enemy_corner_neg() const {
	return Point(enemy_goal().x, - length() / 2);
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
