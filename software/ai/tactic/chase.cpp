#include "ai/tactic/chase.h"
#include "geom/angle.h"

chase::chase(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), move_tactic(new move(ball,field,team,player)) {
}

#include <iostream>

void chase::tick()
{
//	std::cout << (the_ball->position()+the_ball->est_velocity()/2) << std::endl;	
//	std::cout << the_ball->est_velocity() << std::endl;
	move_tactic->set_position(the_ball->position()+the_ball->est_velocity()/2);

	move_tactic->tick();
}
