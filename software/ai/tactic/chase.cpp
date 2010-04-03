#include "ai/tactic/chase.h"
#include "geom/angle.h"

chase::chase(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), move_tactic(new move(ball,field,team,player)) {
}

#include <iostream>

void chase::tick()
{
//	std::cout << (the_ball->position()+the_ball->est_velocity()/2) << std::endl;	
//	std::cout << the_ball->est_velocity() << std::endl;
	//move_tactic->set_position(the_ball->position()+the_ball->est_velocity()/2);
	// predict ball position based on velocity and accleration
 	//NOTE THAT MOVING BALL MANUALLY WITH CURSOR CAN GIVE VERY LARGE VELOCITY AND ACCELERATION
	move_tactic->set_position(the_ball->position()+the_ball->est_velocity()/2 + the_ball->est_acceleration()/8);

	move_tactic->tick();
}
