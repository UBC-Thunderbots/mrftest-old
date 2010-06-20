#include "ai/role/defensive.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/receive.h"
#include "ai/util.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"

#include <iostream>

defensive::defensive(world::ptr world) : the_world(world) {
}

std::vector<point> defensive::calc_block_positions() const {
	const enemy_team& enemy(the_world->enemy);

	// Sort enemies by distance from own goal.
	std::vector<robot::ptr> enemies = enemy.get_robots();
	std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<robot::ptr>(the_world->field().friendly_goal()));

	// Place waypoints on the defence area.
	// TODO: calculate proper areas in the future.
	std::vector<point> waypoints;
	for (size_t i = 0; i < enemies.size(); ++i) {
		point half = (enemies[i]->position() + the_world->field().friendly_goal()) * 0.5;
		waypoints.push_back(half);
	}

	// TODO: have only up to one defensive robot in the defence area.

	return waypoints;
}

void defensive::tick() {

	if (the_robots.size() == 0) return;

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(the_robots.begin(), the_robots.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));

	const friendly_team& friendly(the_world->friendly);
	const int baller = ai_util::calc_baller(the_world, the_robots);
	const bool teampossesball = ai_util::friendly_posses_ball(the_world);

	std::vector<player::ptr> friends = ai_util::get_friends(friendly, the_robots);
	std::sort(friends.begin(), friends.end(), ai_util::cmp_dist<player::ptr>(the_world->field().enemy_goal()));

	// The robot that will do something to the ball (e.g. chase).
	// Other robots will just go defend or something.
	// TODO: maybe use refpointer instead of integer for safety reasons.
	int skipme = -1;

	if (teampossesball) {
		if (baller >= 0) {

			// If a player in the role has a ball, then
			// pass to the other friendly, or wait if there is none.
			int passme = -1;
			for (size_t i = 0; i < friends.size(); ++i) {
				if (friends[i]->position().x < the_robots[baller]->position().x) continue;
				if (ai_util::can_receive(the_world, friends[i])) {
					passme = i;
					break;
				}
			}

			// TODO: do something
			if (passme == -1) {
				LOG_INFO(Glib::ustring::compose("%1 shoot", the_robots[baller]->name));

				// try for the goal =D
				shoot::ptr shoot_tactic(new shoot(the_robots[baller], the_world));
				tactics[baller] = shoot_tactic;
			} else {
				LOG_INFO(Glib::ustring::compose("%1 pass to %2", the_robots[baller]->name, friends[passme]->name));

				// pass to this person
				pass::ptr pass_tactic(new pass(the_robots[baller], the_world, friends[passme]));
				tactics[baller] = pass_tactic;
			}
			skipme = baller;
		} else {

			// If a player nearest to the goal area has the ball
			// that player is probably a goalie, chase the ball!
			std::sort(friends.begin(), friends.end(), ai_util::cmp_dist<player::ptr>(the_world->field().friendly_goal()));

			if (friends.size() > 0 && ai_util::posses_ball(the_world, friends[0])) {
				LOG_INFO(Glib::ustring::compose("%1 get ball from goalie", the_robots[0]->name));
				receive::ptr receive_tactic(new receive(the_robots[0], the_world));
				tactics[0] = receive_tactic;
				skipme = 0;
			}
		}
	} else {

		double frienddist = 1e99;
		for (size_t i = 0; i < friends.size(); ++i) {
			frienddist = std::min(frienddist, (friends[i]->position() - the_world->ball()->position()).len());
		}

		if ((the_robots[0]->position() - the_world->ball()->position()).len() < frienddist) {
			// std::cout << "defensive: chase" << std::endl;

			// already sorted by distance to ball
			shoot::ptr shoot_tactic(new shoot(the_robots[0], the_world));
			// want to get rid of the ball ASAP!
			shoot_tactic->force();
			tactics[0] = shoot_tactic;
			skipme = 0;
		} else {
			// std::cout << "defensive: nothing special" << std::endl;
		}
	}

	std::vector<point> waypoints = calc_block_positions();

	std::vector<player::ptr> available;
	std::vector<point> locations;
	for (size_t i = 0; i < the_robots.size(); ++i) {
		if (static_cast<int>(i) == skipme) continue;
		available.push_back(the_robots[i]);
		locations.push_back(the_robots[i]->position());
	}

	// ensure we are only blocking as we need
	while (waypoints.size() > available.size())
		waypoints.pop_back();

	std::vector<size_t> order = dist_matching(locations, waypoints);

	size_t w = 0;
	for (size_t i = 0; i < the_robots.size(); ++i) {
		if (static_cast<int>(i) == skipme) continue;
		if (w >= waypoints.size()) {
			LOG_WARN(Glib::ustring::compose("%1 nothing to do", the_robots[i]->name));
			move::ptr move_tactic(new move(the_robots[i], the_world));
			move_tactic->set_position(the_robots[i]->position());
			tactics[i] = move_tactic;
		} else {
			move::ptr move_tactic(new move(the_robots[i], the_world));
			move_tactic->set_position(waypoints[order[w]]);
			tactics[i] = move_tactic;
		}
		++w;
	}

	unsigned int flags = ai_flags::calc_flags(the_world->playtype());

	for (size_t i = 0; i < tactics.size(); ++i) {
		if (static_cast<int>(i) == baller) {
			tactics[i]->set_flags(flags | ai_flags::clip_play_area);
		} else {
			tactics[i]->set_flags(flags);
		}
		tactics[i]->tick();
	}
}

void defensive::robots_changed() {
	tactics.clear();
	tactics.resize(the_robots.size());
}

