#include "ai/role/execute_penalty_friendly.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/shoot.h"
#include "ai/util.h"

execute_penalty_friendly::execute_penalty_friendly(world::ptr world) : the_world(world) {
}

void execute_penalty_friendly::tick() {
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());

#warning has ball here
	if (!the_shooter->sense_ball()) {
		chase::ptr tactic(new chase(the_shooter, the_world));
		tactic->set_flags(flags);
		tactic->tick();
	} else {
		const team& enemy_team(the_world->enemy);
		const point goal(the_world->field().length(), 0);

		robot::ptr the_goalie;
		for (unsigned int i = 0; i < enemy_team.size(); ++i) {
			the_goalie = enemy_team.get_robot(i);

			// found the goalie
			if (the_goalie->position().x > the_world->field().length() - PENALTY_MARK_LENGTH) {
			
				// goalie is moving away from the opening
				if ((the_goalie->position().y > 0 && the_goalie->est_velocity().y >= -ai_util::VEL_CLOSE) || 
					(the_goalie->position().y < 0 && the_goalie->est_velocity().y <= ai_util::VEL_CLOSE)) {

					// shoot should automatically find the opening
					shoot::ptr tactic(new shoot(the_shooter, the_world));
					tactic->tick();
				}
				
				// else, let's wait until the goalie stops moving or is moving away from the opening
				break;
			}
		}
	}
}

void execute_penalty_friendly::robots_changed() {
	the_shooter = the_robots[0];
}

