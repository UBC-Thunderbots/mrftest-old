#include "ai/strategy/strategy.h"
#include "ai/role/defensive3.h"
#include "ai/util.h"

namespace {
	class TestDefensive : public Strategy {
		public:
			TestDefensive(RefPtr<World> world);
			void tick();
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
			void player_added(unsigned int, RefPtr<Player> player);
			void player_removed(unsigned int, RefPtr<Player> player);
		private:
			const RefPtr<World> world;
			Defensive3 defensive;
	};

	TestDefensive::TestDefensive(RefPtr<World> world) : world(world), defensive(world) {
		world->friendly.signal_player_added.connect(sigc::mem_fun(this, &TestDefensive::player_added));
		world->friendly.signal_player_removed.connect(sigc::mem_fun(this, &TestDefensive::player_removed));
	}

	void TestDefensive::tick() {
		defensive.tick();
	}

	void TestDefensive::player_added(unsigned int, RefPtr<Player> player) {
		defensive.add_player(player);
	}

	void TestDefensive::player_removed(unsigned int, RefPtr<Player> player) {
		defensive.remove_player(player);
	}

	Gtk::Widget *TestDefensive::get_ui_controls() {
		return NULL;
	}

	class TestDefensiveFactory : public StrategyFactory {
		public:
			TestDefensiveFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	TestDefensiveFactory::TestDefensiveFactory() : StrategyFactory("Test Defensive V3 Role") {
	}

	RefPtr<Strategy2> TestDefensiveFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new TestDefensive(world));
		return s;
	}

	TestDefensiveFactory factory;

	StrategyFactory &TestDefensive::get_factory() {
		return factory;
	}
}

