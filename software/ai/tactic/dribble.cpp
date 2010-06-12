#include "ai/tactic/dribble.h"
#include "ai/util.h"

dribble::dribble(player::ptr player, world::ptr world) : the_world(world), tactic(player), navi(player, world), position_initialized(false), orientation_initialized(false) {
}

void dribble::tick() {
	if (!the_player->sense_ball()) {
		// if the robot loses the ball temporarily, go pick it up
		navi.set_position(the_world->ball()->position());
	} else {
		// maybe i should face enemy goal by default
		if (position_initialized) navi.set_position(target_position);
		if (orientation_initialized) navi.set_orientation(target_orientation);
	}
	navi.set_flags(flags);
	navi.tick();
}

