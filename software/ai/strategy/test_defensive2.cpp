#include "ai/strategy/strategy.h"
#include "ai/role/defensive2.h"
#include "ai/util.h"

namespace {
	class TestDefensive2 : public Strategy2 {
		public:
			TestDefensive2(RefPtr<World> world);
			void tick(Cairo::RefPtr<Cairo::Context> overlay);
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
			const RefPtr<World> the_world;
	};

	TestDefensive2::TestDefensive2(RefPtr<World> world) : the_world(world) {

	}

	void TestDefensive2::tick(Cairo::RefPtr<Cairo::Context> overlay) {
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

	Gtk::Widget *TestDefensive2::get_ui_controls() {
		return 0;
	}

	class TestDefensive2Factory : public StrategyFactory {
		public:
			TestDefensive2Factory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	TestDefensive2Factory::TestDefensive2Factory() : StrategyFactory("Test Defensive V2 Role") {
	}

	RefPtr<Strategy2> TestDefensive2Factory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new TestDefensive2(world));
		return s;
	}

	TestDefensive2Factory factory;

	StrategyFactory &TestDefensive2::get_factory() {
		return factory;
	}
}

