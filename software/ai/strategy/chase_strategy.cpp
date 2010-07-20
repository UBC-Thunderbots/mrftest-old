#include "ai/strategy/strategy.h"
#include "ai/tactic/chase.h"
#include "ai/flags.h"

namespace {
	class ChaseStrategy : public Strategy {
		public:
			ChaseStrategy(RefPtr<World> world);
			void tick();
			void set_playtype(PlayType::PlayType t);
			const RefPtr<World> the_world;
			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
	};

	ChaseStrategy::ChaseStrategy(RefPtr<World> world) : the_world(world) {
	}

	void ChaseStrategy::tick() {
		const FriendlyTeam &the_team(the_world->friendly);
		unsigned int flags = AIFlags::calc_flags(the_world->playtype());
		for (unsigned int i = 0; i < the_team.size(); i++)
		{
			Chase chaser(the_team.get_player(i), the_world);
			chaser.set_flags(flags);
			chaser.tick();
		}
	}

	void ChaseStrategy::set_playtype(PlayType::PlayType) {
	}
	
	Gtk::Widget *ChaseStrategy::get_ui_controls() {
		return 0;
	}

	class ChaseStrategyFactory : public StrategyFactory {
		public:
			ChaseStrategyFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	ChaseStrategyFactory::ChaseStrategyFactory() : StrategyFactory("Chase Strategy") {
	}

	RefPtr<Strategy2> ChaseStrategyFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new ChaseStrategy(world));
		return s;
	}

	ChaseStrategyFactory factory;

	StrategyFactory &ChaseStrategy::get_factory() {
		return factory;
	}
}

