#include "ai/role/goalie.h"
#include "ai/tactic/move.h"

#include <iostream>

namespace {
	const double STANDBY_DIST = 0.2;
}

goalie::goalie(world::ptr world) : the_world(world) {
}

void goalie::tick(){

	if (the_robots.size() < 1) return;
	if (the_robots.size() > 1) {
		std::cerr << "goalie role: multiple robots!" << std::endl;
	}

	const player::ptr me = the_robots[0];
	const friendly_team& friendly(the_world->friendly);

	// ? Chase the ball if need to.
	// std::vector<player::ptr> friends = ai_util::get_players(friendly);
	// std::sort(friends.begin(), friends.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));

	if (me->has_ball()) {
#warning implement
		// TODO: implement
		// Find a friend to pass to.
	} else {
		// Generic defence.
		const point default_pos = point(-0.45*the_world->field().length(), 0);
		const point centre_of_goal = point(-0.5*the_world->field().length(), 0);
		move move_tactic(me, the_world);
		point tempPoint = the_world->ball()->position()-centre_of_goal;
		tempPoint = tempPoint * (STANDBY_DIST / tempPoint.len());
		tempPoint += centre_of_goal;
		move_tactic.set_position(tempPoint);
		move_tactic.tick();
	} 
}

void goalie::robots_changed() {
}

