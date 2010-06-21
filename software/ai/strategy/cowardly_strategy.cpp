#include "ai/strategy/basic_strategy.h"
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
#include "ai/util.h"

#include "uicomponents/param.h"

#include <iostream>

namespace {

	/**
	 * A really basic strategy that satisfies the rule.
	 * Most strategies should derive this function and overide in_play_assignment().
	 * Assumes that the first robot is always the goalie.
	 */
	class cowardly_strategy : public basic_strategy {
		public:
			cowardly_strategy(world::ptr w);

			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();

		protected:

			void in_play_assignment();

			player::ptr minus_one_assignment();
	};

	class cowardly_strategy_factory : public strategy_factory {
		public:
			cowardly_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	cowardly_strategy_factory::cowardly_strategy_factory() : strategy_factory("Cowardly Strategy") {
	}

	strategy::ptr cowardly_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new cowardly_strategy(world));
		return s;
	}

	cowardly_strategy_factory factory;

	cowardly_strategy::cowardly_strategy(world::ptr w) : basic_strategy(w) {
	}

	Gtk::Widget *cowardly_strategy::get_ui_controls() {
		return NULL;
	}

	void cowardly_strategy::in_play_assignment() {
		const friendly_team &the_team(the_world->friendly);

		// TODO: SORT

		roles.clear();
		if (the_team.size() == 0) return;

		defensive2::ptr defensive_role(new defensive2(the_world));
		offensive::ptr offensive_role(new offensive(the_world));
		roles.push_back(role::ptr(defensive_role));
		roles.push_back(role::ptr(offensive_role));
		std::vector<player::ptr> defenders;
		std::vector<player::ptr> offenders;

		defenders.push_back(the_team.get_player(0));

		if (the_team.size() >= 2)
			offenders.push_back(the_team.get_player(1));

		if (the_team.size() >= 3)
			defenders.push_back(the_team.get_player(2));

		if (the_team.size() >= 4)
			defenders.push_back(the_team.get_player(3));

		if (the_team.size() >= 5)
			defenders.push_back(the_team.get_player(4));

		// If we have ball and ball is on non-goalie defender, switch this defender to offender
		// and switch robot 4 back to defender
		if (defenders.size() >= 2 && ai_util::posses_ball(the_world,defenders[1]))
			std::swap(defenders[1],offenders[offenders.size()-1]);
		// extra players become offenders
		for (size_t i = 5; i < the_team.size(); ++i)
			offenders.push_back(the_team.get_player(i));

		defensive_role->set_robots(defenders);
		defensive_role->set_goalie(the_team.get_player(0));
		offensive_role->set_robots(offenders);
	}

	player::ptr cowardly_strategy::minus_one_assignment() {

		// TODO: SORT

		const friendly_team &the_team(the_world->friendly);

		roles.clear();
		if (the_team.size() == 0) return player::ptr();

		if (the_team.size() == 1) return the_team.get_player(0);

		// other players just sort by distance

		defensive2::ptr defensive_role(new defensive2(the_world));
		offensive::ptr offensive_role(new offensive(the_world));
		roles.push_back(role::ptr(defensive_role));
		roles.push_back(role::ptr(offensive_role));
		std::vector<player::ptr> defenders;
		std::vector<player::ptr> offenders;

		defenders.push_back(the_team.get_player(0));
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
		defensive_role->set_goalie(the_team.get_player(0));
		offensive_role->set_robots(offenders);

		return the_team.get_player(1);
	}

	strategy_factory &cowardly_strategy::get_factory() {
		return factory;
	}

}

