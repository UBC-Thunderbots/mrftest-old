#include "ai/tactic/dribble.h"
#include "ai/util.h"

dribble::dribble(player::ptr player, world::ptr world) : tactic(player), the_world(world), navi(player, world), position_initialized(false), orientation_initialized(false) {
}

void dribble::tick() {
	if (!the_player->has_ball()) {
		// if the robot loses the ball temporarily, go pick it up
		navi.set_position(the_world->ball()->position());
	} else {
		// maybe i should face enemy goal by default
		if (the_player->dribble_distance() < player::MAX_DRIBBLE_DIST) {
			if (position_initialized) navi.set_position(target_position);
		}
		if (orientation_initialized) {
			navi.set_orientation(target_orientation);
		} else {
			const point diff = the_world->field().enemy_goal() - the_player->position();
			navi.set_orientation(diff.orientation());
		}
	}
	navi.set_flags(flags);
	navi.tick();
}

