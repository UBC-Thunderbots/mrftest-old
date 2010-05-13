#include "ai/tactic/pass_receive.h"

pass_receive::pass_receive(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player, player::ptr passer) : tactic(ball, field, team, player), turn_tactic(new turn(ball, field, team, player)) {
	the_passer = passer;
}

void pass_receive::tick()
{
	// if this robot is to receive the pass, just stand still and turn towards the passer
	turn_tactic->set_direction(the_passer->position());
	turn_tactic->tick();
}
