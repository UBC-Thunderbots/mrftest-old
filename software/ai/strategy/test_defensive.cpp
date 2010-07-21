#include "ai/strategy/strategy.h"
#include "ai/role/defensive3.h"
#include "ai/util.h"

namespace {
	class TestDefensive : public Strategy {
		public:
			TestDefensive(World::ptr world);
			void tick();
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
			void player_added(unsigned int, Player::ptr player);
			void player_removed(unsigned int, Player::ptr player);
		private:
			const World::ptr world;
			Defensive3 defensive;
	};

	TestDefensive::TestDefensive(World::ptr world) : world(world), defensive(world) {
		world->friendly.signal_player_added.connect(sigc::mem_fun(this, &TestDefensive::player_added));
		world->friendly.signal_player_removed.connect(sigc::mem_fun(this, &TestDefensive::player_removed));
	}

	void TestDefensive::tick() {
		defensive.tick();
	}

	void TestDefensive::player_added(unsigned int, Player::ptr player) {
		defensive.add_player(player);
	}

	void TestDefensive::player_removed(unsigned int, Player::ptr player) {
		defensive.remove_player(player);
	}

	Gtk::Widget *TestDefensive::get_ui_controls() {
		return NULL;
	}

	class TestDefensiveFactory : public StrategyFactory {
		public:
			TestDefensiveFactory();
			Strategy2::ptr create_strategy(World::ptr world);
	};

	TestDefensiveFactory::TestDefensiveFactory() : StrategyFactory("Test Defensive V3 Role") {
	}

	Strategy2::ptr TestDefensiveFactory::create_strategy(World::ptr world) {
		Strategy2::ptr s(new TestDefensive(world));
		return s;
	}

	TestDefensiveFactory factory;

	StrategyFactory &TestDefensive::get_factory() {
		return factory;
	}
}

