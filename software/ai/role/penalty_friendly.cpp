#include "ai/role/penalty_friendly.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/move.h"
#include "ai/flags.h"

#include <iostream>

penalty_friendly::penalty_friendly(world::ptr world) : the_world(world) {
	const field& the_field(world->field());

	// Let the first robot to be always the shooter
	ready_positions[0] = point(0.5 * the_field.length() - PENALTY_MARK_LENGTH - robot::MAX_RADIUS, 0);

	ready_positions[1] = point(0, 0);

	// Let two robots be on the offensive, in case there is a rebound
	ready_positions[2] = point(0.5 * the_field.length() - RESTRICTED_ZONE_LENGTH - robot::MAX_RADIUS, -5 * robot::MAX_RADIUS);
	ready_positions[3] = point(0.5 * the_field.length() - RESTRICTED_ZONE_LENGTH - robot::MAX_RADIUS, 5 * robot::MAX_RADIUS);

}

void penalty_friendly::tick() {
	if (the_robots.size() == 0) {
		std::cerr << "penalty_friendly: no robots " << std::endl;
		return;
	}

	// flags... hopefully set correct
	const unsigned int flags = ai_flags::calc_flags(the_world->playtype());

	if (the_world->playtype() == playtype::prepare_penalty_friendly) {
		for (size_t i = 0; i < the_robots.size(); ++i) {
			move tactic(the_robots[i], the_world);
			tactic.set_position(ready_positions[i]);
			if (i) tactic.set_flags(flags);
			else tactic.set_flags((flags & ~ai_flags::avoid_ball_stop) | ai_flags::avoid_ball_near);
			tactic.tick();
		}
	} else if (the_world->playtype() == playtype::execute_penalty_friendly) {

		// make shooter shoot
		const player::ptr shooter = the_robots[0];
		shoot tactic(shooter, the_world);
		tactic.force();
		// don't set flags, otherwise robot will try to avoid ball
		tactic.tick();

		for (size_t i = 1; i < the_robots.size(); ++i) {
			move tactic(the_robots[i], the_world);
			tactic.set_position(ready_positions[i]);
			tactic.set_flags(flags);
			tactic.tick();
		}
	} else {
		std::cerr << "penalty_friendly: unhandled playtype" << std::endl;
	}
}

void penalty_friendly::robots_changed() {

}

