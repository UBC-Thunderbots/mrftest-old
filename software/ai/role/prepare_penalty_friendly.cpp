#include "ai/role/prepare_penalty_friendly.h"

PreparePenaltyFriendly::PreparePenaltyFriendly(World::ptr world) : the_world(world) {
	const Field& the_field(world->field());

	// Let the first robot to be always the shooter
	ready_positions[0] = Point(0.5 * the_field.length() - PENALTY_MARK_LENGTH - Robot::MAX_RADIUS, 0);
		
	// Let two robots be on the offensive, in case there is a rebound
	ready_positions[1] = Point(0.5 * the_field.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);
	ready_positions[2] = Point(0.5 * the_field.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);

	ready_positions[3] = Point(0, 0);
}

void PreparePenaltyFriendly::tick() {
	unsigned int flags = AIFlags::calc_flags(the_world->playtype());
	for (unsigned int i = 0; i < the_tactics.size(); ++i) {
		the_tactics[i]->set_flags(flags);
		the_tactics[i]->tick();
	}
}

void PreparePenaltyFriendly::robots_changed() {
	the_tactics.clear();

	for (unsigned int i = 0; i < robots.size(); ++i) {
		Move::ptr tactic(new Move(robots[i], the_world));
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

