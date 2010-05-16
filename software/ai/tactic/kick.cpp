#include "ai/tactic/kick.h"

#include <iostream>

namespace {
	const double TOL = 10;
}

kick::kick(player::ptr player) : the_player(player), turn_tactic(player), should_chip(false), chip_strength(1.0), kick_strength(1.0) {
}

void kick::tick() {
	// turn towards the target
	turn_tactic.set_direction(the_target);

	if (!turn_tactic.is_turned(TOL)) {
		turn_tactic.tick();	
		return;
	}

	if (should_chip) {
		if (the_player->has_ball()){
			the_player->chip(chip_strength);
		}
	} else {
		if (the_player->has_ball()){
			the_player->kick(kick_strength);
		}
	}
}

