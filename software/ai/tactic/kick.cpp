#include "ai/tactic/kick.h"

kick::kick(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), turn_tactic(new turn(ball, field, team, player)) {
}

void kick::set_target(const point& p) {
	the_target = p;
}

void kick::set_chip(const bool& chip) {
	should_chip = chip;
}

void kick::tick() {
	// turn towards the target
	turn_tactic->set_direction(the_target);
	turn_tactic->tick();	

	// assume maximum strength for now...
	if (should_chip)
		the_player->chip(1);
	else
		the_player->kick(1);
}

