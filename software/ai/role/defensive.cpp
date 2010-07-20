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

Defensive::Defensive(RefPtr<World> world) : the_world(world) {
}

std::vector<Point> Defensive::calc_block_positions() const {
	const EnemyTeam& enemy(the_world->enemy);

	// Sort enemies by distance from own goal.
	std::vector<RefPtr<Robot> > enemies = enemy.get_robots();
	std::sort(enemies.begin(), enemies.end(), AIUtil::CmpDist<RefPtr<Robot> >(the_world->field().friendly_goal()));

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
	tactics.clear();
	tactics.resize(players.size());

	if (players.size() == 0) return;

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(players.begin(), players.end(), AIUtil::CmpDist<RefPtr<Player> >(the_world->ball()->position()));

	const FriendlyTeam& friendly(the_world->friendly);
	const int baller = AIUtil::calc_baller(the_world, players);
	const bool teampossesball = AIUtil::friendly_posses_ball(the_world);

	std::vector<RefPtr<Player> > friends = AIUtil::get_friends(friendly, players);
	std::sort(friends.begin(), friends.end(), AIUtil::CmpDist<RefPtr<Player> >(the_world->field().enemy_goal()));

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
				if (friends[i]->position().x < players[baller]->position().x) continue;
				if (AIUtil::can_receive(the_world, friends[i])) {
					passme = i;
					break;
				}
			}

			// TODO: do something
			if (passme == -1) {
				LOG_INFO(Glib::ustring::compose("%1 shoot", players[baller]->name));

				// try for the goal =D
				RefPtr<Shoot> shoot_tactic(new Shoot(players[baller], the_world));
				tactics[baller] = shoot_tactic;
			} else {
				LOG_INFO(Glib::ustring::compose("%1 pass to %2", players[baller]->name, friends[passme]->name));

				// pass to this person
				RefPtr<Pass> pass_tactic(new Pass(players[baller], the_world, friends[passme]));
				tactics[baller] = pass_tactic;
			}
			skipme = baller;
		} else {

			// If a player nearest to the goal area has the ball
			// that player is probably a goalie, chase the ball!
			std::sort(friends.begin(), friends.end(), AIUtil::CmpDist<RefPtr<Player> >(the_world->field().friendly_goal()));

			if (friends.size() > 0 && AIUtil::posses_ball(the_world, friends[0])) {
				LOG_INFO(Glib::ustring::compose("%1 get ball from goalie", players[0]->name));
				RefPtr<Receive> receive_tactic(new Receive(players[0], the_world));
				tactics[0] = receive_tactic;
				skipme = 0;
			}
		}
	} else {

		double frienddist = 1e99;
		for (size_t i = 0; i < friends.size(); ++i) {
			frienddist = std::min(frienddist, (friends[i]->position() - the_world->ball()->position()).len());
		}

		if ((players[0]->position() - the_world->ball()->position()).len() < frienddist) {
			// std::cout << "defensive: chase" << std::endl;

			RefPtr<Shoot> shoot_tactic = RefPtr<Shoot>(new Shoot(players[0], the_world));

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

	std::vector<RefPtr<Player> > available;
	std::vector<Point> locations;
	for (size_t i = 0; i < players.size(); ++i) {
		if (static_cast<int>(i) == skipme) continue;
		available.push_back(players[i]);
		locations.push_back(players[i]->position());
	}

	// ensure we are only blocking as we need
	while (waypoints.size() > available.size())
		waypoints.pop_back();

	std::vector<size_t> order = dist_matching(locations, waypoints);

	size_t w = 0;
	for (size_t i = 0; i < players.size(); ++i) {
		if (static_cast<int>(i) == skipme) continue;
		if (w >= waypoints.size()) {
			LOG_WARN(Glib::ustring::compose("%1 nothing to do", players[i]->name));
			RefPtr<Move> move_tactic(new Move(players[i], the_world));
			move_tactic->set_position(players[i]->position());
			tactics[i] = move_tactic;
		} else {
			RefPtr<Move> move_tactic(new Move(players[i], the_world));
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

void Defensive::players_changed() {
}

