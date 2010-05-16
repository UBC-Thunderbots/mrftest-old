#include "ai/tactic/chase.h"
#include "geom/angle.h"

chase::chase(player::ptr player, world::ptr world) : the_player(player), the_world(world), move_tactic(player, world) {
}

#include <iostream>

void chase::tick()
{
	const ball::ptr the_ball(the_world->ball());
//	std::cout << (the_ball->position()+the_ball->est_velocity()/2) << std::endl;	
//	std::cout << the_ball->est_velocity() << std::endl;
	//move_tactic->set_position(the_ball->position()+the_ball->est_velocity()/2);
	// predict ball position based on velocity and accleration
 	//NOTE THAT MOVING BALL MANUALLY WITH CURSOR CAN GIVE VERY LARGE VELOCITY AND ACCELERATION
	move_tactic.set_position(the_ball->position()+the_ball->est_velocity()/2 + the_ball->est_acceleration()/8);

	move_tactic.tick();
}
