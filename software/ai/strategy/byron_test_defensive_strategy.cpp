#include "ai/strategy/strategy.h"
#include "ai/role/byrons_defender.h"
#include "ai/role/goalie.h"
#include "ai/util.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class TestDefensiveStrategy : public Strategy2 {
		public:
			TestDefensiveStrategy(World::ptr world);
			void tick(Cairo::RefPtr<Cairo::Context> overlay);
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const World::ptr the_world;
	};

	TestDefensiveStrategy::TestDefensiveStrategy(World::ptr world) : the_world(world) {

	}

	void TestDefensiveStrategy::tick(Cairo::RefPtr<Cairo::Context> overlay) {
		if (the_world->playtype() == PlayType::HALT) {
			return;
		}
		const FriendlyTeam &the_team(the_world->friendly);
		if (the_team.size() == 0) return;

		const Ball::ptr the_ball(the_world->ball());
		ByronsDefender defensive_role(the_world);
		Goalie goalie_role(the_world);
		std::vector<Player::ptr> defenders;
		std::vector<Player::ptr> goalies;

		goalies.push_back(the_team.get_player(0));

		for (size_t i = 1; i < the_team.size(); ++i) {
			defenders.push_back(the_team.get_player(i));
		}

		goalie_role.set_players(goalies);
		defensive_role.set_players(defenders);
		goalie_role.tick();
		defensive_role.tick();
	}

	Gtk::Widget *TestDefensiveStrategy::get_ui_controls() {
		return 0;
	}

	class TestDefensiveStrategyFactory : public StrategyFactory {
		public:
			TestDefensiveStrategyFactory();
			Strategy::ptr create_strategy(World::ptr world);
	};

	TestDefensiveStrategyFactory::TestDefensiveStrategyFactory() : StrategyFactory("Byron's Test Defender Strategy") {
	}

	Strategy::ptr TestDefensiveStrategyFactory::create_strategy(World::ptr world) {
		Strategy::ptr s(new TestDefensiveStrategy(world));
		return s;
	}

	TestDefensiveStrategyFactory factory;

	StrategyFactory &TestDefensiveStrategy::get_factory() {
		return factory;
	}
}

