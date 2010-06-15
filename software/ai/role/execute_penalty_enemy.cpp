#include <vector>
#include "ai/role/execute_penalty_enemy.h"
#include "ai/util.h"

execute_penalty_enemy::execute_penalty_enemy(world::ptr world) : the_world(world), 
	starting_position(-0.5 * the_world->field().length(), -0.5 * the_world->field().goal_width() + robot::MAX_RADIUS),
	ending_position(-0.5 * the_world->field().length(), 0.5 * the_world->field().goal_width() - robot::MAX_RADIUS) {
	should_patrol = true;
	should_go_to_start = true;
}

void execute_penalty_enemy::tick(){
	if (should_patrol) {
		patrol_tactic->tick();
	} else {
		// not at ready position yet
		if ((the_goalie->position() - starting_position).lensq() > ai_util::POS_CLOSE) {
			move_to_start->tick();
		} else {
			if (the_goalie->orientation() > ai_util::ORI_CLOSE) {
				// turn towards enemy goal
				the_goalie->move(the_goalie->position(), 0);
			}
	
			if (!enemy_moved) {
				// check whether the shooter has begun shooting the ball
				enemy_moved = detect_enemy_movement();
			}
		} 

		// if the shooter has made a move, move to the other corner right away
		if (enemy_moved) {
			move_to_end->tick();
		}
	}
}

void execute_penalty_enemy::robots_changed() {	
	the_goalie = the_robots[0];

	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	flags &= ~(ai_flags::avoid_friendly_defence);
	flags &= ~(ai_flags::penalty_kick_enemy);

	move_to_start = move::ptr(new move(the_goalie, the_world, flags));
	move_to_start->set_position(starting_position);

	move_to_end = move::ptr(new move(the_goalie, the_world, flags));
	move_to_end->set_position(ending_position);

	patrol_tactic = patrol::ptr(new patrol(the_goalie, the_world, flags, starting_position, ending_position));
}

bool execute_penalty_enemy::detect_enemy_movement() {
	
	if (!the_shooter) {
		const team& enemy = the_world->enemy;
		const point the_goal(-0.5 * the_world->field().length(), 0);

		std::vector<int> robots;

		// gets the shooter by looking at the closest enemy robot
		for (unsigned int i = 0; i < enemy.size(); ++i) {
			robot::ptr robot = enemy.get_robot(i);
			double dist = fabs(-0.5 * the_world->field().length() - robot->position().x);

			if (dist >= PENALTY_MARK_LENGTH && dist <= RESTRICTED_ZONE_LENGTH) {
				robots.push_back(i);
			}
		}

		// there should be only 1 robot within the shooting area
		if (robots.size() == 1) {
			the_shooter = enemy.get_robot(robots[0]);
			last_orientation = the_shooter->orientation();
		}
	}
	
	// the_shooter may be just populated 
	if (!the_shooter) {
		bool ret = the_shooter->orientation() - last_orientation > ai_util::ORI_CLOSE;
		last_orientation = the_shooter->orientation();
		return ret;
	}

	// return false if it's not set up yet
	return false;
}

