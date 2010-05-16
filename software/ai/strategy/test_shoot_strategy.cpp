#include "ai/strategy/strategy.h"
#include "ai/tactic/shoot.h"
#include <iostream>

namespace {
	class test_shoot_strategy : public strategy {
		public:
			test_shoot_strategy(world::ptr world);
			void tick();
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const world::ptr the_world;
	};

	test_shoot_strategy::test_shoot_strategy(world::ptr world) : the_world(world) {

	}

	void test_shoot_strategy::tick() {

		player::ptr shooter = the_world->friendly.get_player(0);
		shoot shoot_tactic(shooter, the_world);
		
		shoot_tactic.tick();	
	}

	Gtk::Widget *test_shoot_strategy::get_ui_controls() {
		return 0;
	}

	class test_shoot_strategy_factory : public strategy_factory {
		public:
			test_shoot_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	test_shoot_strategy_factory::test_shoot_strategy_factory() : strategy_factory("Test(Shoot) Strategy") {
	}

	strategy::ptr test_shoot_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new test_shoot_strategy(world));
		return s;
	}

	test_shoot_strategy_factory factory;

	strategy_factory &test_shoot_strategy::get_factory() {
		return factory;
	}
}

