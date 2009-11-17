#include "ai/tactic/move_between_robots.h"

move_between_robots::move_between_robots(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) , move_between_tactic(new move_between(ball, field, team, player)) {
	
}

void move_between_robots::set_robots(robot::ptr robotA, robot::ptr robotB) {
	the_robot1 = robotA;
	the_robot2 = robotB;
}

void move_between_robots::tick() {
	move_between_tactic->set_points(the_robot1->position(), the_robot2->position());
	move_between_tactic->tick();
}
