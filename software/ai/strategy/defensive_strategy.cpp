#include "ai/strategy.h"

//this seems to have virtually nothing but the framework, add more stuff when roles are available

namespace {
	class defensive_strategy : public virtual strategy {
		public:
			defensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			virtual void update();
			virtual void set_playtype(playtype::playtype t);
			virtual strategy_factory &get_factory();
			virtual Gtk::Widget *get_ui_controls();

		private:
			playtype::playtype current_playtype;

			// Create variables here (e.g. to store the roles).
	};

	defensive_strategy::defensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
		// Initialize variables here (e.g. create the roles).
	}

	void defensive_strategy::update() {
		// Use the variables "ball", "field", and "team" to allocate players to roles.
	}

	void defensive_strategy::set_playtype(playtype::playtype t) {
		current_playtype = t;
	}

	Gtk::Widget *defensive_strategy::get_ui_controls() {
		return 0;
	}

	class defensive_strategy_factory : public virtual strategy_factory {
		public:
			defensive_strategy_factory();
			virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
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
