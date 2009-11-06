#include "ai/strategy.h"
//this seems to have virtually nothing but the framework, add more stuff when roles are available

namespace {
	class defensive_strategy : public strategy {
		public:
			defensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, robot::ptr r);

		private:
			playtype::playtype current_playtype;

			// Create variables here (e.g. to store the roles).
	};

	defensive_strategy::defensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
		// Initialize variables here (e.g. create the roles).
	}

	void defensive_strategy::tick() {
		// Use the variables "ball", "field", and "team" to allocate players to roles.
	}

	void defensive_strategy::set_playtype(playtype::playtype t) {
		current_playtype = t;
	}
	
	Gtk::Widget *defensive_strategy::get_ui_controls() {
		return 0;
	}

  void defensive_strategy::robot_added(void){
  }

  void defensive_strategy::robot_removed(unsigned int index, robot::ptr r){
  }

	class defensive_strategy_factory : public strategy_factory {
		public:
			defensive_strategy_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	defensive_strategy_factory::defensive_strategy_factory() : strategy_factory("defensive Strategy") {
	}

	strategy::ptr defensive_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new defensive_strategy(ball, field, team));
		return s;
	}

	defensive_strategy_factory factory;

	strategy_factory &defensive_strategy::get_factory() {
		return factory;
	}
}
