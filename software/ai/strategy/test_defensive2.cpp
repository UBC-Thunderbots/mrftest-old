#include "ai/strategy/strategy.h"
#include "ai/role/defensive2.h"
#include "ai/util.h"

namespace {
	class TestDefensive2 : public Strategy2 {
		public:
			TestDefensive2(World::ptr world);
			void tick(Cairo::RefPtr<Cairo::Context> overlay);
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const World::ptr the_world;
	};

	TestDefensive2::TestDefensive2(World::ptr world) : the_world(world) {

	}

	void TestDefensive2::tick(Cairo::RefPtr<Cairo::Context> overlay) {
		if (the_world->playtype() == PlayType::HALT) {
			return;
		}
		const FriendlyTeam &the_team(the_world->friendly);
		if (the_team.size() == 0) return;

		const Ball::ptr the_ball(the_world->ball());
		Defensive2 defensive2_role(the_world);
		std::vector<Player::ptr> all;
		for (size_t i = 0; i < the_team.size(); ++i) {
			all.push_back(the_team[i]);
		}
		defensive2_role.set_goalie(the_team[0]);
		defensive2_role.set_players(all);
		defensive2_role.tick();
	}

	Gtk::Widget *TestDefensive2::get_ui_controls() {
		return 0;
	}

	class TestDefensive2Factory : public StrategyFactory {
		public:
			TestDefensive2Factory();
			Strategy2::ptr create_strategy(World::ptr world);
	};

	TestDefensive2Factory::TestDefensive2Factory() : StrategyFactory("Test Defensive V2 Role") {
	}

	Strategy2::ptr TestDefensive2Factory::create_strategy(World::ptr world) {
		Strategy2::ptr s(new TestDefensive2(world));
		return s;
	}

	TestDefensive2Factory factory;

	StrategyFactory &TestDefensive2::get_factory() {
		return factory;
	}
}

