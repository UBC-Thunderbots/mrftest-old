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
	 * A really basic Strategy that satisfies the rule.
	 * Most strategies should derive this function and overide in_play_assignment().
	 * Assumes that the first robot is always the goalie.
	 */
	class CowardlyStrategy : public BasicStrategy {
		public:
			CowardlyStrategy(RefPtr<World> w);

			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();

		protected:

			void in_play_assignment();

			RefPtr<Player> minus_one_assignment();
	};

	class CowardlyStrategyFactory : public StrategyFactory {
		public:
			CowardlyStrategyFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	CowardlyStrategyFactory::CowardlyStrategyFactory() : StrategyFactory("Cowardly Strategy") {
	}

	RefPtr<Strategy2> CowardlyStrategyFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new CowardlyStrategy(world));
		return s;
	}

	CowardlyStrategyFactory factory;

	CowardlyStrategy::CowardlyStrategy(RefPtr<World> w) : BasicStrategy(w) {
	}

	Gtk::Widget *CowardlyStrategy::get_ui_controls() {
		return NULL;
	}

	void CowardlyStrategy::in_play_assignment() {
		const FriendlyTeam &the_team(the_world->friendly);

		// TODO: SORT

		roles.clear();
		if (the_team.size() == 0) return;

		RefPtr<Defensive2> defensive_role(new Defensive2(the_world));
		RefPtr<Offensive> offensive_role(new Offensive(the_world));
		roles.push_back(RefPtr<Role>(defensive_role));
		roles.push_back(RefPtr<Role>(offensive_role));
		std::vector<RefPtr<Player> > defenders;
		std::vector<RefPtr<Player> > offenders;

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
		if (defenders.size() >= 2 && AIUtil::posses_ball(the_world,defenders[1]))
			std::swap(defenders[1],offenders[offenders.size()-1]);
		// extra players become offenders
		for (size_t i = 5; i < the_team.size(); ++i)
			offenders.push_back(the_team.get_player(i));

		defensive_role->set_robots(defenders);
		defensive_role->set_goalie(the_team.get_player(0));
		offensive_role->set_robots(offenders);
	}

	RefPtr<Player> CowardlyStrategy::minus_one_assignment() {

		// TODO: SORT

		const FriendlyTeam &the_team(the_world->friendly);

		roles.clear();
		if (the_team.size() == 0) return RefPtr<Player>();

		if (the_team.size() == 1) return the_team.get_player(0);

		// other players just sort by distance

		RefPtr<Defensive2> defensive_role(new Defensive2(the_world));
		RefPtr<Offensive> offensive_role(new Offensive(the_world));
		roles.push_back(RefPtr<Role>(defensive_role));
		roles.push_back(RefPtr<Role>(offensive_role));
		std::vector<RefPtr<Player> > defenders;
		std::vector<RefPtr<Player> > offenders;

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

	StrategyFactory &CowardlyStrategy::get_factory() {
		return factory;
	}

}

