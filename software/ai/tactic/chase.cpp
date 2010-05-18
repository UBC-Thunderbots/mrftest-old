#include "ai/tactic/chase.h"
#include "geom/angle.h"

#include <iostream>

namespace {
	double VEL_FACTOR = 0.5;
	double ACL_FACTOR = 0.125;
}

chase::chase(player::ptr player, world::ptr world) : the_player(player), the_world(world), move_tactic(player, world) {
}

void chase::tick() {
	const ball::ptr the_ball(the_world->ball());
//	std::cout << (the_ball->position()+the_ball->est_velocity()/2) << std::endl;	
//	std::cout << the_ball->est_velocity() << std::endl;
	//move_tactic->set_position(the_ball->position()+the_ball->est_velocity()/2);
	// predict ball position based on velocity and accleration
 	//NOTE THAT MOVING BALL MANUALLY WITH CURSOR CAN GIVE VERY LARGE VELOCITY AND ACCELERATION
	move_tactic.set_position(the_ball->position() + VEL_FACTOR * the_ball->est_velocity() + ACL_FACTOR * the_ball->est_acceleration());
	move_tactic.tick();
}

