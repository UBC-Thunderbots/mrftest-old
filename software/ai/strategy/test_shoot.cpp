#include "ai/strategy/strategy.h"
#include "ai/tactic/shoot.h"
#include <iostream>

namespace {
	class TestShootStrategy : public Strategy {
		public:
			TestShootStrategy(World::Ptr world);
			void tick();
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const World::Ptr the_world;
	};

	TestShootStrategy::TestShootStrategy(World::Ptr world) : the_world(world) {

	}

	void TestShootStrategy::tick() {

		if (the_world->friendly.size() == 0) return;

		Player::Ptr shooter = the_world->friendly.get_player(0);
		Shoot shoot_tactic(shooter, the_world);
		
		shoot_tactic.tick();	
	}

	Gtk::Widget *TestShootStrategy::get_ui_controls() {
		return 0;
	}

	class TestShootStrategyFactory : public StrategyFactory {
		public:
			TestShootStrategyFactory();
			Strategy2::Ptr create_strategy(World::Ptr world);
	};

	TestShootStrategyFactory::TestShootStrategyFactory() : StrategyFactory("Test Shoot Tactic") {
	}

	Strategy2::Ptr TestShootStrategyFactory::create_strategy(World::Ptr world) {
		Strategy2::Ptr s(new TestShootStrategy(world));
		return s;
	}

	TestShootStrategyFactory factory;

	StrategyFactory &TestShootStrategy::get_factory() {
		return factory;
	}
}

