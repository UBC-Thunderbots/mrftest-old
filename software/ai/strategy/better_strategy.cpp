#include "ai/strategy/better_strategy.h"
#include "ai/tactic/chase.h"
#include "ai/role/defensive2.h"
#include "ai/role/goalie.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_free_kick.h" 
#include "ai/role/pit_stop.h"   
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/kickoff_friendly.h"             
#include "ai/role/penalty_friendly.h"
#include "ai/role/penalty_enemy.h"
#include "ai/role/victory_dance.h"


#include <iostream>

namespace {

	class better_strategy_factory : public strategy_factory {
		public:
			better_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	better_strategy_factory::better_strategy_factory() : strategy_factory("Better than Basic Strategy") {
	}

	strategy::ptr better_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new better_strategy(world));
		return s;
	}

	better_strategy_factory factory;
}

better_strategy::better_strategy(world::ptr w) : basic_strategy(w) {
}
	
Gtk::Widget *better_strategy::get_ui_controls() {
	return NULL;
}

void better_strategy::in_play_assignment() {
	const friendly_team &the_team(the_world->friendly);

	roles.clear();
	if (the_team.size() == 0) return;

	goalie::ptr goalie_role(new goalie(the_world));
	roles.push_back(role::ptr(goalie_role));
	std::vector<player::ptr> goalie;
	goalie.push_back(the_team.get_player(0));
	goalie_role->set_robots(goalie);

#warning TODO

	defensive2::ptr defensive_role(new defensive2(the_world));
	offensive::ptr offensive_role(new offensive(the_world));
	roles.push_back(role::ptr(defensive_role));
	roles.push_back(role::ptr(offensive_role));
	std::vector<player::ptr> defenders;
	std::vector<player::ptr> offenders;

	if (the_team.size() >= 2)
		offenders.push_back(the_team.get_player(1));

	if (the_team.size() >= 3)
		defenders.push_back(the_team.get_player(2));

	if (the_team.size() >= 4)
		offenders.push_back(the_team.get_player(3));

	if (the_team.size() >= 5)
		defenders.push_back(the_team.get_player(4));
	
	// extra players become offenders
	for (size_t i = 5; i < the_team.size(); ++i)
		offenders.push_back(the_team.get_player(i));

	defensive_role->set_robots(defenders);
	offensive_role->set_robots(offenders);
}

player::ptr better_strategy::minus_one_assignment() {
	const friendly_team &the_team(the_world->friendly);

	roles.clear();
	if (the_team.size() == 0) return player::ptr();

	if (the_team.size() == 1) return the_team.get_player(0);

	goalie::ptr goalie_role(new goalie(the_world));
	roles.push_back(role::ptr(goalie_role));
	std::vector<player::ptr> goalie;
	goalie.push_back(the_team.get_player(0));
	goalie_role->set_robots(goalie);

	// other players just sort by distance

	defensive2::ptr defensive_role(new defensive2(the_world));
	offensive::ptr offensive_role(new offensive(the_world));
	roles.push_back(role::ptr(defensive_role));
	roles.push_back(role::ptr(offensive_role));
	std::vector<player::ptr> defenders;
	std::vector<player::ptr> offenders;

	if (the_team.size() >= 3)
		defenders.push_back(the_team.get_player(2));

	if (the_team.size() >= 4)
		offenders.push_back(the_team.get_player(3));

	if (the_team.size() >= 5)
		defenders.push_back(the_team.get_player(4));
	
	// extra players become offenders
	for (size_t i = 5; i < the_team.size(); ++i)
		offenders.push_back(the_team.get_player(i));

	defensive_role->set_robots(defenders);
	offensive_role->set_robots(offenders);

	return the_team.get_player(1);
}

strategy_factory &better_strategy::get_factory() {
	return factory;
}

