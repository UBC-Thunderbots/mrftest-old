#include "ai/strategy/strategy.h"
#include "ai/role/byrons_defender.h"
#include "ai/role/goalie.h"
#include "ai/util.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class test_defensive_strategy : public strategy2 {
		public:
			test_defensive_strategy(world::ptr world);
			void tick(Cairo::RefPtr<Cairo::Context> overlay);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const world::ptr the_world;
	};

	test_defensive_strategy::test_defensive_strategy(world::ptr world) : the_world(world) {

	}

	void test_defensive_strategy::tick(Cairo::RefPtr<Cairo::Context> overlay) {
		if (the_world->playtype() == playtype::halt) {
			return;
		}
		const friendly_team &the_team(the_world->friendly);
		if (the_team.size() == 0) return;

		const ball::ptr the_ball(the_world->ball());
		byrons_defender defensive_role(the_world);
		goalie goalie_role(the_world);
		std::vector<player::ptr> defenders;
		std::vector<player::ptr> goalies;

		goalies.push_back(the_team.get_player(0));

		for (size_t i = 1; i < the_team.size(); ++i) {
			defenders.push_back(the_team.get_player(i));
		}

		goalie_role.set_robots(goalies);
		defensive_role.set_robots(defenders);
		goalie_role.tick();
		defensive_role.tick();
	}

	Gtk::Widget *test_defensive_strategy::get_ui_controls() {
		return 0;
	}

	class test_defensive_strategy_factory : public strategy_factory {
		public:
			test_defensive_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	test_defensive_strategy_factory::test_defensive_strategy_factory() : strategy_factory("Byron's Test Defender Strategy") {
	}

	strategy::ptr test_defensive_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new test_defensive_strategy(world));
		return s;
	}

	test_defensive_strategy_factory factory;

	strategy_factory &test_defensive_strategy::get_factory() {
		return factory;
	}
}

