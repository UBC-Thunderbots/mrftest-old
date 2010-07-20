#include "ai/strategy/strategy.h"
#include "ai/tactic/shoot.h"
#include <iostream>

namespace {
	class TestShootStrategy : public Strategy {
		public:
			TestShootStrategy(RefPtr<World> world);
			void tick();
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const RefPtr<World> the_world;
	};

	TestShootStrategy::TestShootStrategy(RefPtr<World> world) : the_world(world) {

	}

	void TestShootStrategy::tick() {

		if (the_world->friendly.size() == 0) return;

		RefPtr<Player> shooter = the_world->friendly.get_player(0);
		Shoot shoot_tactic(shooter, the_world);
		
		shoot_tactic.tick();	
	}

	Gtk::Widget *TestShootStrategy::get_ui_controls() {
		return 0;
	}

	class TestShootStrategyFactory : public StrategyFactory {
		public:
			TestShootStrategyFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	TestShootStrategyFactory::TestShootStrategyFactory() : StrategyFactory("Test(Shoot) Strategy") {
	}

	RefPtr<Strategy2> TestShootStrategyFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new TestShootStrategy(world));
		return s;
	}

	TestShootStrategyFactory factory;

	StrategyFactory &TestShootStrategy::get_factory() {
		return factory;
	}
}

