#include "ai/strategy/strategy.h"
#include "ai/role/offensive.h"
#include "ai/util.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class TestOffensiveStrategy : public Strategy2 {
		public:
			TestOffensiveStrategy(World::ptr world);
			void tick(Cairo::RefPtr<Cairo::Context> overlay);
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const World::ptr the_world;
	};

	TestOffensiveStrategy::TestOffensiveStrategy(World::ptr world) : the_world(world) {

	}

	void TestOffensiveStrategy::tick(Cairo::RefPtr<Cairo::Context> overlay) {
		if (the_world->playtype() == PlayType::HALT) {
			return;
		}
		const FriendlyTeam &the_team(the_world->friendly);
		const Ball::ptr the_ball(the_world->ball());
		Offensive offensive_role(the_world);
		std::vector<Player::ptr> offenders;

		for (size_t i = 0; i < the_team.size(); ++i) {
			offenders.push_back(the_team.get_player(i));
		}

		offensive_role.set_robots(offenders);
		offensive_role.tick();
	}

	Gtk::Widget *TestOffensiveStrategy::get_ui_controls() {
		return 0;
	}

	class TestOffensiveStrategyFactory : public StrategyFactory {
		public:
			TestOffensiveStrategyFactory();
			Strategy::ptr create_strategy(World::ptr world);
	};

	TestOffensiveStrategyFactory::TestOffensiveStrategyFactory() : StrategyFactory("Test(Offensive) Strategy") {
	}

	Strategy::ptr TestOffensiveStrategyFactory::create_strategy(World::ptr world) {
		Strategy::ptr s(new TestOffensiveStrategy(world));
		return s;
	}

	TestOffensiveStrategyFactory factory;

	StrategyFactory &TestOffensiveStrategy::get_factory() {
		return factory;
	}
}

