#include "ai/role/prepare_penalty_enemy.h"

prepare_penalty_enemy::prepare_penalty_enemy(world::ptr world) : the_world(world) {
	const field& the_field(the_world->field());

	standing_positions[0] = point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + robot::MAX_RADIUS, 5 * robot::MAX_RADIUS);
	standing_positions[1] = point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + robot::MAX_RADIUS, 2 * robot::MAX_RADIUS);
	standing_positions[2] = point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + robot::MAX_RADIUS, -2 * robot::MAX_RADIUS);
	standing_positions[3] = point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + robot::MAX_RADIUS, -5 * robot::MAX_RADIUS);
}

void prepare_penalty_enemy::tick(){
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	for (unsigned int i = 0; i < the_tactics.size(); ++i) {
		the_tactics[i]->set_flags(flags);
		the_tactics[i]->tick();
	}
}

void prepare_penalty_enemy::robots_changed() {
	the_tactics.clear();

	for (unsigned int i = 0; i < the_robots.size(); ++i) {
		if (i < NUM_POSITIONS) {
			move::ptr tactic(new move(the_robots[i], the_world));
			tactic->set_position(standing_positions[i]);	
			the_tactics.push_back(tactic);
		}
	}	
}

