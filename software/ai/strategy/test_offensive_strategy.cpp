#include "ai/strategy/strategy.h"
#include "ai/role/offensive.h"
#include "ai/util.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class test_offensive_strategy : public strategy2 {
		public:
			test_offensive_strategy(world::ptr world);
			void tick(Cairo::RefPtr<Cairo::Context> overlay);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const world::ptr the_world;
	};

	test_offensive_strategy::test_offensive_strategy(world::ptr world) : the_world(world) {

	}

	void test_offensive_strategy::tick(Cairo::RefPtr<Cairo::Context> overlay) {
		if (the_world->playtype() == playtype::halt) {
			return;
		}
		const friendly_team &the_team(the_world->friendly);
		const ball::ptr the_ball(the_world->ball());
		offensive offensive_role(the_world);
		std::vector<player::ptr> offenders;

		for (size_t i = 0; i < the_team.size(); ++i) {
			offenders.push_back(the_team.get_player(i));
		}

		offensive_role.set_robots(offenders);
		offensive_role.tick(overlay);
	}

	Gtk::Widget *test_offensive_strategy::get_ui_controls() {
		return 0;
	}

	class test_offensive_strategy_factory : public strategy_factory {
		public:
			test_offensive_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	test_offensive_strategy_factory::test_offensive_strategy_factory() : strategy_factory("Test(Offensive) Strategy") {
	}

	strategy::ptr test_offensive_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new test_offensive_strategy(world));
		return s;
	}

	test_offensive_strategy_factory factory;

	strategy_factory &test_offensive_strategy::get_factory() {
		return factory;
	}
}

