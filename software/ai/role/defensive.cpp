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

Defensive::Defensive(World::ptr world) : the_world(world) {
}

std::vector<Point> Defensive::calc_block_positions() const {
	const EnemyTeam& enemy(the_world->enemy);

	// Sort enemies by distance from own goal.
	std::vector<Robot::ptr> enemies = enemy.get_robots();
	std::sort(enemies.begin(), enemies.end(), AIUtil::CmpDist<Robot::ptr>(the_world->field().friendly_goal()));

	// Place waypoints on the defence area.
	// TODO: calculate proper areas in the future.
	std::vector<Point> waypoints;
	for (size_t i = 0; i < enemies.size(); ++i) {
		Point half = (enemies[i]->position() + the_world->field().friendly_goal()) * 0.5;
		waypoints.push_back(half);
	}

	// TODO: have only up to one defensive robot in the defence area.

	return waypoints;
}

void Defensive::tick() {

	if (robots.size() == 0) return;

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(robots.begin(), robots.end(), AIUtil::CmpDist<Player::ptr>(the_world->ball()->position()));

	const FriendlyTeam& friendly(the_world->friendly);
	const int baller = AIUtil::calc_baller(the_world, robots);
	const bool teampossesball = AIUtil::friendly_posses_ball(the_world);

	std::vector<Player::ptr> friends = AIUtil::get_friends(friendly, robots);
	std::sort(friends.begin(), friends.end(), AIUtil::CmpDist<Player::ptr>(the_world->field().enemy_goal()));

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
				if (friends[i]->position().x < robots[baller]->position().x) continue;
				if (AIUtil::can_receive(the_world, friends[i])) {
					passme = i;
					break;
				}
			}

			// TODO: do something
			if (passme == -1) {
				LOG_INFO(Glib::ustring::compose("%1 shoot", robots[baller]->name));

				// try for the goal =D
				Shoot::ptr shoot_tactic(new Shoot(robots[baller], the_world));
				tactics[baller] = shoot_tactic;
			} else {
				LOG_INFO(Glib::ustring::compose("%1 pass to %2", robots[baller]->name, friends[passme]->name));

				// pass to this person
				Pass::ptr pass_tactic(new Pass(robots[baller], the_world, friends[passme]));
				tactics[baller] = pass_tactic;
			}
			skipme = baller;
		} else {

			// If a player nearest to the goal area has the ball
			// that player is probably a goalie, chase the ball!
			std::sort(friends.begin(), friends.end(), AIUtil::CmpDist<Player::ptr>(the_world->field().friendly_goal()));

			if (friends.size() > 0 && AIUtil::posses_ball(the_world, friends[0])) {
				LOG_INFO(Glib::ustring::compose("%1 get ball from goalie", robots[0]->name));
				Receive::ptr receive_tactic(new Receive(robots[0], the_world));
				tactics[0] = receive_tactic;
				skipme = 0;
			}
		}
	} else {

		double frienddist = 1e99;
		for (size_t i = 0; i < friends.size(); ++i) {
			frienddist = std::min(frienddist, (friends[i]->position() - the_world->ball()->position()).len());
		}

		if ((robots[0]->position() - the_world->ball()->position()).len() < frienddist) {
			// std::cout << "defensive: chase" << std::endl;

			Shoot::ptr shoot_tactic = Shoot::ptr(new Shoot(robots[0], the_world));

			// want to get rid of the ball ASAP!
			shoot_tactic->force();
			shoot_tactic->set_pivot(false);
			tactics[0] = shoot_tactic;
			skipme = 0;
		} else {
			// std::cout << "defensive: nothing special" << std::endl;
		}
	}

	std::vector<Point> waypoints = calc_block_positions();

	std::vector<Player::ptr> available;
	std::vector<Point> locations;
	for (size_t i = 0; i < robots.size(); ++i) {
		if (static_cast<int>(i) == skipme) continue;
		available.push_back(robots[i]);
		locations.push_back(robots[i]->position());
	}

	// ensure we are only blocking as we need
	while (waypoints.size() > available.size())
		waypoints.pop_back();

	std::vector<size_t> order = dist_matching(locations, waypoints);

	size_t w = 0;
	for (size_t i = 0; i < robots.size(); ++i) {
		if (static_cast<int>(i) == skipme) continue;
		if (w >= waypoints.size()) {
			LOG_WARN(Glib::ustring::compose("%1 nothing to do", robots[i]->name));
			Move::ptr move_tactic(new Move(robots[i], the_world));
			move_tactic->set_position(robots[i]->position());
			tactics[i] = move_tactic;
		} else {
			Move::ptr move_tactic(new Move(robots[i], the_world));
			move_tactic->set_position(waypoints[order[w]]);
			tactics[i] = move_tactic;
		}
		++w;
	}

	unsigned int flags = AIFlags::calc_flags(the_world->playtype());

	for (size_t i = 0; i < tactics.size(); ++i) {
		if (static_cast<int>(i) == baller) {
			tactics[i]->set_flags(flags | AIFlags::CLIP_PLAY_AREA);
		} else {
			tactics[i]->set_flags(flags);
		}
		tactics[i]->tick();
	}
}

void Defensive::robots_changed() {
	tactics.clear();
	tactics.resize(robots.size());
}

