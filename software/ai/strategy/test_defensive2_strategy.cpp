#include "ai/strategy/strategy.h"
#include "ai/role/defensive2.h"
#include "ai/role/goalie.h"
#include "ai/util.h"
#include <iostream>
#include "time.h"
#include <cstdlib>

namespace {
	class TestDefensive2Strategy : public Strategy2 {
		public:
			TestDefensive2Strategy(RefPtr<World> world);
			void tick(Cairo::RefPtr<Cairo::Context> overlay);
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const RefPtr<World> the_world;
	};

	TestDefensive2Strategy::TestDefensive2Strategy(RefPtr<World> world) : the_world(world) {

	}

	void TestDefensive2Strategy::tick(Cairo::RefPtr<Cairo::Context> overlay) {
		if (the_world->playtype() == PlayType::HALT) {
			return;
		}
		const FriendlyTeam &the_team(the_world->friendly);
		if (the_team.size() == 0) return;

		const RefPtr<Ball> the_ball(the_world->ball());
		Defensive2 defensive2_role(the_world);
		std::vector<RefPtr<Player> > all;
		for (size_t i = 0; i < the_team.size(); ++i) {
			all.push_back(the_team[i]);
		}
		defensive2_role.set_goalie(the_team[0]);
		defensive2_role.set_robots(all);
		defensive2_role.tick();
	}

	Gtk::Widget *TestDefensive2Strategy::get_ui_controls() {
		return 0;
	}

	class TestDefensive2StrategyFactory : public StrategyFactory {
		public:
			TestDefensive2StrategyFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	TestDefensive2StrategyFactory::TestDefensive2StrategyFactory() : StrategyFactory("Test(Defensive V2) Strategy") {
	}

	RefPtr<Strategy2> TestDefensive2StrategyFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new TestDefensive2Strategy(world));
		return s;
	}

	TestDefensive2StrategyFactory factory;

	StrategyFactory &TestDefensive2Strategy::get_factory() {
		return factory;
	}
}

