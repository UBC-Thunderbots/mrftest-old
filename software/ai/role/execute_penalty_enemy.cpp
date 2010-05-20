#include <vector>
#include "ai/role/execute_penalty_enemy.h"
#include "ai/util.h"

execute_penalty_enemy::execute_penalty_enemy(world::ptr world) : the_world(world), 
	starting_position(-the_world->field().length(), -the_world->field().goal_width() + robot::MAX_RADIUS),
	ending_position(-the_world->field().length(), the_world->field().goal_width() - robot::MAX_RADIUS) {
	should_patrol = true;
	should_go_to_start = true;
}

void execute_penalty_enemy::tick(){
	if (should_patrol) {
		patrol();		
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

	move_to_start = move::ptr(new move(the_goalie, the_world));
	move_to_start->set_position(starting_position);

	move_to_end = move::ptr(new move(the_goalie, the_world));
	move_to_end->set_position(ending_position);
}

void execute_penalty_enemy::patrol() {
	if (should_go_to_start) {
		// reached the start position
		if ((the_goalie->position() - starting_position).lensq() <= ai_util::POS_CLOSE) {
			should_go_to_start = false;
			move_to_end->tick();
		} else {
			move_to_start->tick();
		}
	} else {
		// reached the end position
		if ((the_goalie->position() - ending_position).lensq() <= ai_util::POS_CLOSE) {
			should_go_to_start = true;
			move_to_start->tick();
		} else {
			move_to_end->tick();
		}
	}
}

bool execute_penalty_enemy::detect_enemy_movement() {
	
	if (!the_shooter) {
		const team& enemy = the_world->enemy;
		const point the_goal(-the_world->field().length(),0);

		vector<int> robots;

		// gets the shooter by looking at the closest enemy robot
		for (unsigned int i = 0; i < enemy.size(); ++i) {
			robot::ptr robot = enemy.get_robot(i);
			double dist = fabs(-the_world->field().length() - robot->position().x);

			if (dist >= penalty_mark_length && dist <= restricted_zone_length) {
				dists.push_back(i);
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

