#include "ai/tactic/chase.h"

chase::chase(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), move_tactic(new move(ball,field,team,player)) {
}

void chase::tick()
{
	move_tactic->set_position(the_ball->position());
	move_tactic->tick();
}
