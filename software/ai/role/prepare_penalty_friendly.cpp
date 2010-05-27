#include "ai/role/prepare_penalty_friendly.h"

prepare_penalty_friendly::prepare_penalty_friendly(world::ptr world) : the_world(world) {
	const field& the_field(world->field());

	// Let the first robot to be always the shooter
	ready_positions[0] = point(the_field.length() - PENALTY_MARK_LENGTH - robot::MAX_RADIUS, 0);
		
	// Let two robots be on the offensive, in case there is a rebound
	ready_positions[1] = point(the_field.length() - RESTRICTED_ZONE_LENGTH - robot::MAX_RADIUS, -5 * robot::MAX_RADIUS);
	ready_positions[2] = point(the_field.length() - RESTRICTED_ZONE_LENGTH - robot::MAX_RADIUS, 5 * robot::MAX_RADIUS);

	ready_positions[3] = point(0, 0);
}

void prepare_penalty_friendly::tick() {
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	for (unsigned int i = 0; i < the_tactics.size(); ++i) {
		the_tactics[i]->set_flags(flags);
		the_tactics[i]->tick();
	}
}

void prepare_penalty_friendly::robots_changed() {
	the_tactics.clear();

	for (unsigned int i = 0; i < the_robots.size(); ++i) {
		move::ptr tactic(new move(the_robots[i], the_world));
		the_tactics.push_back(tactic);

		// if shooter
		if (i == 0) {
			// turn towards the goal
			tactic->set_orientation(0);
		}

		if (i < NUMBER_OF_READY_POSITIONS) {			
			tactic->set_position(ready_positions[i]);
		}
	}	
}

