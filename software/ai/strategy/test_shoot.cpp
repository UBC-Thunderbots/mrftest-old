#include "ai/strategy/strategy.h"
#include "ai/tactic/shoot.h"
#include <iostream>

namespace {
	class TestShootStrategy : public Strategy {
		public:
			TestShootStrategy(World::ptr world);
			void tick();
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const World::ptr the_world;
	};

	TestShootStrategy::TestShootStrategy(World::ptr world) : the_world(world) {

	}

	void TestShootStrategy::tick() {

		if (the_world->friendly.size() == 0) return;

		Player::ptr shooter = the_world->friendly.get_player(0);
		Shoot shoot_tactic(shooter, the_world);
		
		shoot_tactic.tick();	
	}

	Gtk::Widget *TestShootStrategy::get_ui_controls() {
		return 0;
	}

	class TestShootStrategyFactory : public StrategyFactory {
		public:
			TestShootStrategyFactory();
			Strategy2::ptr create_strategy(World::ptr world);
	};

	TestShootStrategyFactory::TestShootStrategyFactory() : StrategyFactory("Test Shoot Tactic") {
	}

	Strategy2::ptr TestShootStrategyFactory::create_strategy(World::ptr world) {
		Strategy2::ptr s(new TestShootStrategy(world));
		return s;
	}

	TestShootStrategyFactory factory;

	StrategyFactory &TestShootStrategy::get_factory() {
		return factory;
	}
}

