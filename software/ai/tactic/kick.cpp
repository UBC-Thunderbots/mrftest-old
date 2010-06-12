#include "ai/tactic/kick.h"
#include "ai/util.h"
#include "geom/angle.h"

#include <iostream>

/*
TODO:
- check if the capacitor is ready to allow a kick.
- calculate the strength that the robot should use.
 */

kick::kick(player::ptr player, world::ptr world) : tactic(player), the_world(world), navi(player, world), should_chip(false), chip_strength(1.0), kick_strength(1.0), target_initialized(false) {
}

void kick::tick() {

	// don't forget
	navi.set_flags(flags);

	if (!ai_util::has_ball(the_player)) {
		navi.set_position(the_world->ball()->position());
		navi.tick();
		return;
	}

	if (!target_initialized) {
		std::cerr << "kick: no target specified" << std::endl;
		navi.tick();
		return;
	}

	point dist = kick_target - the_player->position();

	// turn towards the target
	navi.set_orientation(dist.orientation());

	// maybe move towards it?
	// navi.set_position(kick_target);

	if (angle_diff(dist.orientation(), the_player->orientation()) > ai_util::ORI_CLOSE) {
		navi.tick();
		return;
	}
	
	std::cout << "kick: shoot!" << std::endl;

	if (should_chip) {
		the_player->chip(chip_strength);
	} else {
		the_player->kick(kick_strength);
	}

	navi.tick();
}

