#include "ai/tactic/kick.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

#include "ai/navigator/robot_navigator.h"

#include <iostream>

/*
TODO:
- check if the capacitor is ready to allow a kick.
- calculate the strength that the robot should use.
 */

Kick::Kick(Player::Ptr player, World::Ptr world) : Tactic(player), the_world(world), should_chip(false), strength(1.0), kick_target(the_world->field().enemy_goal()) {
}

void Kick::tick() {
	RobotNavigator navi(player, the_world);

	// don't forget
	navi.set_flags(flags);

	if (!AIUtil::has_ball(the_world, player)) {
		navi.set_position(the_world->ball()->position());
		navi.tick();
		return;
	}

	Point dist = kick_target - player->position();

	// turn towards the target
	navi.set_orientation(dist.orientation());

	// maybe move towards it?
	//if (player->dribble_distance() < Player::MAX_DRIBBLE_DIST) {
		//navi.set_position(kick_target);
	//}

	const double anglediff = angle_diff(dist.orientation(), player->orientation());
	if (anglediff > AIUtil::ORI_CLOSE) {
		LOG_DEBUG(Glib::ustring::compose("%1 aiming angle_diff is %2", player->name, anglediff));
		navi.tick();
		return;
	}

	if (player->chicker_ready_time() == 0) {
		if (should_chip) {
			LOG_INFO(Glib::ustring::compose("%1 kick", player->name));
			player->chip(strength);
		} else {
			LOG_INFO(Glib::ustring::compose("%1 chip", player->name));
			player->kick(strength);
		}
	} else {
		LOG_INFO(Glib::ustring::compose("%1 chicker not ready", player->name));
	}

	navi.tick();
}

