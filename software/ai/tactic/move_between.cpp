#include "ai/tactic/move_between.h"

move_between::move_between(player::ptr player, world::ptr world) : tactic(player), move_tactic(player, world), is_initialized(false) {
}

void move_between::set_points(const point& p1, const point& p2) {
	is_initialized = true;
	the_point1 = p1;
	the_point2 = p2;
}

point move_between::calculate_position() {
	point line = the_point2 - the_point1;
	point pos = the_player->position() - the_point1;
	double proj = pos.dot(line.norm());

	// closest point in between the two points will be point 1
	if (proj < 0) {
		return the_point1 + line.norm() * 2 * robot::MAX_RADIUS;
	} 

	line = the_point1 - the_point2;
	pos = the_player->position() - the_point2;
	proj = pos.dot(line.norm());

	// closest point in between the two points will be point 2
	if (proj < 0) {
		return the_point2 + line.norm() * 2 * robot::MAX_RADIUS;
	}

	// closest point is somewhere in between, go to point with shortest perpendicular distance
	return the_point2 + line.norm() * proj / line.len();
}

void move_between::tick() {
	if (is_initialized) {
		move_tactic.set_position(calculate_position());
		move_tactic.set_flags(flags);
		move_tactic.tick();
	}
}
