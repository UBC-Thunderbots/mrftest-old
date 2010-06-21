#include "ai/tactic/kick.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

#include <iostream>

/*
TODO:
- check if the capacitor is ready to allow a kick.
- calculate the strength that the robot should use.
 */

kick::kick(player::ptr player, world::ptr world) : tactic(player), the_world(world), navi(player, world), should_chip(false), strength(1.0), kick_target(the_world->field().enemy_goal()) {
}

void kick::tick() {

	// don't forget
	navi.set_flags(flags);

	if (!ai_util::has_ball(the_world, the_player)) {
		navi.set_position(the_world->ball()->position());
		navi.tick();
		return;
	}

	point dist = kick_target - the_player->position();

	// turn towards the target
	navi.set_orientation(dist.orientation());

	// maybe move towards it?
	if (the_player->dribble_distance() < player::MAX_DRIBBLE_DIST) {
		navi.set_position(kick_target);
	}

	const double anglediff = angle_diff(dist.orientation(), the_player->orientation());
	if (anglediff > ai_util::ORI_CLOSE) {
		LOG_DEBUG(Glib::ustring::compose("%1 aiming angle_diff is %2", the_player->name, anglediff));
		navi.tick();
		return;
	}
	
	if (the_player->chicker_ready_time() == 0) {
		if (should_chip) {
			LOG_INFO(Glib::ustring::compose("%1 kick", the_player->name));
			the_player->chip(strength);
		} else {
			LOG_INFO(Glib::ustring::compose("%1 chip", the_player->name));
			the_player->kick(strength);
		}
	} else {
		LOG_INFO(Glib::ustring::compose("%1 chicker not ready", the_player->name));
	}

	navi.tick();
}

