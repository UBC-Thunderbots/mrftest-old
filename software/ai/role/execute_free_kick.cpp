#include "ai/role/execute_free_kick.h"
#include "ai/util.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/pivot.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/kick.h"
#include "util/dprint.h"

#include <vector>
#include <iostream>

ExecuteIndirectFreeKick::ExecuteIndirectFreeKick(World::ptr world) : the_world(world) {
}

void ExecuteIndirectFreeKick::tick() {
	const FriendlyTeam& friendly(the_world->friendly);

	if (players.size() != 1) {
		std::cerr << "execute_indirect_free_kick: contains " << players.size() << " size " << std::endl;
	}

	if (players.size() == 0) return;

	const Player::ptr pl = players[0];

	std::vector<Player::ptr> friends;
	for (size_t i = 0; i < friendly.size(); ++i)
		if (friendly[i] != pl)
			friends.push_back(friendly[i]);

	// NO FLAGS

	// find someone to pass to
	int bestpassee = AIUtil::choose_best_pass(the_world, friends);

	//if (the_world->playtype_time() > AIUtil::PLAYTYPE_WAIT_TIME) {
		LOG_INFO("forced kicking");
		//Kick tactic(pl, the_world);
		Shoot tactic(pl, the_world);
		tactic.force();
		tactic.tick();
	/*0} else if (bestpassee >= 0) {
		// yup... pass to someone
		LOG_WARN(Glib::ustring::compose("%1 pass to %2", pl->name, friends[bestpassee]->name));
		Pass tactic(pl, the_world, friends[bestpassee]);
		tactic.tick();
	} else {
		Pivot tactic(pl, the_world);
		tactic.set_target(the_world->field().enemy_goal());
		tactic.tick();
	}*/
}

void ExecuteIndirectFreeKick::players_changed() {
}

ExecuteDirectFreeKick::ExecuteDirectFreeKick(World::ptr world) : the_world(world) {
}

void ExecuteDirectFreeKick::tick() {
	const FriendlyTeam& friendly(the_world->friendly);

	if (players.size() != 1) {
		LOG_ERROR(Glib::ustring::compose("there are %1 robots", players.size()));
	}

	if (players.size() == 0) return;

	const Player::ptr pl = players[0];

	std::vector<Player::ptr> friends;
	for (size_t i = 0; i < friendly.size(); ++i)
		if (friendly[i] != pl)
			friends.push_back(friendly[i]);

	// NO FLAGS

	std::pair<Point, double> bestshot = AIUtil::calc_best_shot(the_world, pl);

	// find someone to pass to
	int bestpassee = AIUtil::choose_best_pass(the_world, friends);

	//if (the_world->playtype_time() > AIUtil::PLAYTYPE_WAIT_TIME) {
		// err... something random?
		LOG_INFO("forced kicking");
		//Kick tactic(pl, the_world);
		Shoot tactic(pl, the_world);
		tactic.force();
		tactic.tick();
	/*} else if (bestshot.second > 0) {
		LOG_INFO("shoot");
		Shoot tactic(pl, the_world);
		tactic.tick();
	} else if (bestpassee >= 0) {
		// yup... pass to someone
		LOG_INFO(Glib::ustring::compose("%1 pass to %2", pl->name, friends[bestpassee]->name));
		pass tactic(pl, the_world, friends[bestpassee]);
		tactic.tick();
	} else {
		Pivot tactic(pl, the_world);
		tactic.set_target(the_world->field().enemy_goal());
		tactic.set_flags(flags);
		tactic.tick();
	}*/
}

void ExecuteDirectFreeKick::players_changed() {
}

