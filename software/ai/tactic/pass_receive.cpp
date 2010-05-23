#include "ai/tactic/pass_receive.h"

pass_receive::pass_receive(player::ptr player, player::ptr passer) : the_passer(passer), turn_tactic(player) {
}

void pass_receive::tick()
{
	// if this robot is to receive the pass, just stand still and turn towards the passer
	// Remember to set flags if some other tactic (instead of turn) is called!
	turn_tactic.set_direction(the_passer->position());
	turn_tactic.tick();
}
