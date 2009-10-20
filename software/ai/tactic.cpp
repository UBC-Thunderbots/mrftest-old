#include "ai/tactic.h"



tactic::tactic(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : the_ball(ball), the_field(field), the_team(team), the_player(player) {
}


void tactic::update()
{
	for (unsigned int i = 0; i < sub_tactics.size(); ++i)
		sub_tactics[i]->update();
}

