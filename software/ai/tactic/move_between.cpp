#include "ai/tactic/move_between.h"

move_between::move_between(player::ptr player, world::ptr world) : move_tactic(player, world) {
	
}

void move_between::set_points(const point& p1, const point& p2) {
	the_point1 = p1;
	the_point2 = p2;
}

point move_between::calculate_position() {

	// TODO: Right now, we move towards the mid point, but anywhere in between is fine
	return (the_point1 + the_point2) / 2;
}

void move_between::tick() {
	move_tactic.set_position(calculate_position());
	move_tactic.set_flags(flags);
	move_tactic.tick();
}
