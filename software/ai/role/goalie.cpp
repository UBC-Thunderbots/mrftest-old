#include "ai/util.h"
#include "ai/role/goalie.h"
#include "ai/tactic/move.h"
#include "ai/tactic/pass.h"

#include <iostream>

namespace {
	const double STANDBY_DIST = 0.2;
}

goalie::goalie(world::ptr world) : the_world(world) {
}

void goalie::tick() {
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	flags &= ~(ai_flags::avoid_friendly_defence);
	
	if (the_robots.size() < 1) return;
	if (the_robots.size() > 1) {
		std::cerr << "goalie role: multiple robots!" << std::endl;
	}

	const player::ptr me = the_robots[0];

	if (ai_util::posses_ball(the_world, me)) {
		// TODO: check correctness
		// Code copied from defensive role

		std::vector<player::ptr> friends = ai_util::get_friends(the_world->friendly, the_robots);
		std::sort(friends.begin(), friends.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));

		int nearidx = -1;
		for (size_t i = 0; i < friends.size(); ++i) {
			if (!ai_util::can_receive(the_world, friends[i])) continue;
			nearidx = i;
			break;
		}

		if (nearidx == -1) {
			move move_tactic(me, the_world);
			move_tactic.set_position(me->position());
			move_tactic.set_flags(flags);
			move_tactic.tick();
		} else {
			pass pass_tactic(me, the_world, friends[nearidx]);
			pass_tactic.set_flags(flags);
			pass_tactic.tick();
		}

#warning the goalie cant hold the ball for too long, it should chip somewhere very randomly

	} else {
		// Generic defence.
		const point default_pos = point(-0.45*the_world->field().length(), 0);
		const point centre_of_goal = point(-0.5*the_world->field().length(), 0);
		move move_tactic(me, the_world);
		point tempPoint = the_world->ball()->position()-centre_of_goal;
		tempPoint = tempPoint * (STANDBY_DIST / tempPoint.len());
		tempPoint += centre_of_goal;
		move_tactic.set_position(tempPoint);
		move_tactic.set_flags(flags);
		move_tactic.tick();
	} 
}

void goalie::robots_changed() {
}

