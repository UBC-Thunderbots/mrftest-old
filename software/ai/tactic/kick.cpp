#include "ai/tactic/kick.h"
#include "ai/util.h"

#include <iostream>

namespace {
	const double TOL = 10;
}

/*

TODO:
- disable kicking the air once testing is finished
- what happens if the robot does not have the ball? should this be an error?
- check if the capacitor is ready to allow a kick.
- calculate the strength that the robot should use.
- remove turn tactic, simply use move tactic

*/

kick::kick(player::ptr player) : the_player(player), turn_tactic(player), should_chip(false), chip_strength(1.0), kick_strength(1.0), target_initialized(false) {
}

void kick::tick() {

	if (!the_player->has_ball()) {
		std::cerr << "kick tactic: robot does not have the ball and yet it tries to kick!?" << std::endl;
		// TODO: for the sake of testing, enable kicking the air
	}

	if (!target_initialized) {
		std::cerr << "kick tactic: robot does not know where to kick?" << std::endl;
		return;
	}

	// turn towards the target
	turn_tactic.set_direction(kick_target);
	// if (angle_diff(kick_target.orientation(), the_player->orientation()) < ai_util::ORI_CLOSE) {
	if (!turn_tactic.is_turned(TOL)) {
		turn_tactic.tick();	
		return;
	}

	if (should_chip) {
		the_player->chip(chip_strength);
	} else {
		the_player->kick(kick_strength);
	}
}

