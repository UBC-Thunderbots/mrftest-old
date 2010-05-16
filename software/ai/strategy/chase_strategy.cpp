#include "ai/strategy/strategy.h"
#include "ai/tactic/chase.h"

namespace {
	class chase_strategy : public strategy {
		public:
			chase_strategy(world::ptr world);
			void tick();
			void set_playtype(playtype::playtype t);
			const world::ptr the_world;
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
		private:
	};

	chase_strategy::chase_strategy(world::ptr world) : the_world(world) {
	}

	void chase_strategy::tick() {
		const friendly_team &the_team(the_world->friendly);
		for (unsigned int i = 0; i < the_team.size(); i++)
		{
			chase chaser(the_team.get_player(i), the_world);
			chaser.tick();
		}
	}

	void chase_strategy::set_playtype(playtype::playtype) {
	}
	
	Gtk::Widget *chase_strategy::get_ui_controls() {
		return 0;
	}

	class chase_strategy_factory : public strategy_factory {
		public:
			chase_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	chase_strategy_factory::chase_strategy_factory() : strategy_factory("Chase Strategy") {
	}

	strategy::ptr chase_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new chase_strategy(world));
		return s;
	}

	chase_strategy_factory factory;

	strategy_factory &chase_strategy::get_factory() {
		return factory;
	}
}

