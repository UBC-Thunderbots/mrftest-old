#include "ai/role/execute_free_kick.h"
#include "ai/util.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/pivot.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/kick.h"
#include "util/dprint.h"

#include <vector>
#include <iostream>

execute_indirect_free_kick::execute_indirect_free_kick(world::ptr world) : the_world(world) {
}

void execute_indirect_free_kick::tick() {
	const friendly_team& friendly(the_world->friendly);

	if (the_robots.size() != 1) {
		std::cerr << "execute_indirect_free_kick: contains " << the_robots.size() << " size " << std::endl;
	}

	if (the_robots.size() == 0) return;

	const player::ptr pl = the_robots[0];

	std::vector<player::ptr> friends;
	for (size_t i = 0; i < friendly.size(); ++i)
		if (friendly[i] != pl)
			friends.push_back(friendly[i]);

	//unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	//flags &= ~(ai_flags::clip_play_area);
	unsigned int flags = 0;

	// find someone to pass to
	int bestpassee = ai_util::choose_best_pass(the_world, friends);

	if (the_world->playtype_time() > ai_util::PLAYTYPE_WAIT_TIME) {
		// err... something random?
		LOG_INFO("forced kicking");
		kick tactic(pl, the_world);
		tactic.set_flags(flags);
		tactic.tick();
	} else if (bestpassee >= 0) {
		// yup... pass to someone
		LOG_WARN(Glib::ustring::compose("%1 pass to %2", pl->name, friends[bestpassee]->name));
		pass tactic(pl, the_world, friends[bestpassee]);
		tactic.set_flags(flags);
		tactic.tick();
	} else {
		pivot tactic(pl, the_world);
		tactic.set_target(the_world->field().enemy_goal());
		tactic.set_flags(flags);
		tactic.tick();
	}
}

void execute_indirect_free_kick::robots_changed() {
}

execute_direct_free_kick::execute_direct_free_kick(world::ptr world) : the_world(world) {
}

void execute_direct_free_kick::tick() {
	const friendly_team& friendly(the_world->friendly);

	if (the_robots.size() != 1) {
		LOG_ERROR(Glib::ustring::compose("there are %1 robots", the_robots.size()));
	}

	if (the_robots.size() == 0) return;

	const player::ptr pl = the_robots[0];

	std::vector<player::ptr> friends;
	for (size_t i = 0; i < friendly.size(); ++i)
		if (friendly[i] != pl)
			friends.push_back(friendly[i]);

	//unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	//flags &= ~(ai_flags::clip_play_area);
	unsigned int flags = 0;

	std::pair<point, double> bestshot = ai_util::calc_best_shot(the_world, pl);

	// find someone to pass to
	int bestpassee = ai_util::choose_best_pass(the_world, friends);

	if (the_world->playtype_time() > ai_util::PLAYTYPE_WAIT_TIME) {
		// err... something random?
		LOG_INFO("forced kicking");
		kick tactic(pl, the_world);
		tactic.set_flags(flags);
		tactic.tick();
	} else if (bestshot.second > 0) {
		LOG_INFO("shoot");
		shoot tactic(pl, the_world);
		tactic.set_flags(flags);
		tactic.tick();
	} else if (bestpassee >= 0) {
		// yup... pass to someone
		LOG_INFO(Glib::ustring::compose("%1 pass to %2", pl->name, friends[bestpassee]->name));
		pass tactic(pl, the_world, friends[bestpassee]);
		tactic.set_flags(flags);
		tactic.tick();
	} else {
		pivot tactic(pl, the_world);
		tactic.set_target(the_world->field().enemy_goal());
		tactic.set_flags(flags);
		tactic.tick();
	}
}

void execute_direct_free_kick::robots_changed() {
}

