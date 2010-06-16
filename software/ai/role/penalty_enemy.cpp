#include "ai/role/penalty_enemy.h"
#include "ai/tactic/move.h"
#include "ai/tactic/patrol.h"

#include <iostream>

penalty_enemy::penalty_enemy(world::ptr world) : the_world(world) {
	const field& the_field(the_world->field());

	standing_positions[0] = point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + robot::MAX_RADIUS, 5 * robot::MAX_RADIUS);
	standing_positions[1] = point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + robot::MAX_RADIUS, 2 * robot::MAX_RADIUS);
	standing_positions[2] = point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + robot::MAX_RADIUS, -2 * robot::MAX_RADIUS);
	standing_positions[3] = point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + robot::MAX_RADIUS, -5 * robot::MAX_RADIUS);
}

void penalty_enemy::tick() {
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	for (unsigned int i = 0; i < the_robots.size(); ++i) {
		move tactic(the_robots[i], the_world);
		tactic.set_position(standing_positions[i]);
		tactic.set_flags(flags);
		tactic.tick();
	}
}

void penalty_enemy::robots_changed() {
}

penalty_goalie::penalty_goalie(world::ptr world) : the_world(world) {
}

void penalty_goalie::tick() {
	if (the_robots.size() != 1) {
		std::cerr << "penalty_enemy: we only want 1 goalie!" << std::endl;
	}

	if (the_robots.size() == 0) return;

	const field& f = the_world->field();
	const point starting_position(-0.5 * f.length(), -0.5 * f.goal_width() + robot::MAX_RADIUS);
	const point ending_position(-0.5 * f.length(), 0.5 * f.goal_width() - robot::MAX_RADIUS);

	unsigned int flags = 0;
	patrol tactic(the_robots[0], the_world, flags, starting_position, ending_position);
	tactic.tick();
}

void penalty_goalie::robots_changed() {
}

