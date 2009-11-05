#include "ai/strategy.h"
#include <iostream>

//this seems to have virtually nothing but the framework, add more stuff when roles are available

namespace {
	class chase_strategy : public virtual strategy {
		public:
			chase_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			virtual void update();
			virtual void set_playtype(playtype::playtype t);
			virtual strategy_factory &get_factory();
			virtual Gtk::Widget *get_ui_controls();
			virtual void robot_added(void);
			virtual void robot_removed(unsigned int index, robot::ptr r);

		private:
			playtype::playtype current_playtype;

			// Create variables here (e.g. to store the roles).
	};

	chase_strategy::chase_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
		std::cout << "Chase strategy constructing\n";
		// Initialize variables here (e.g. create the roles).
	}

	void chase_strategy::update() {
		// Use the variables "ball", "field", and "team" to allocate players to roles.
		switch (current_playtype) {
		case playtype::halt:;
		case playtype::stop:;
		case playtype::play:;
		case playtype::prepare_kickoff_friendly:;
		case playtype::execute_kickoff_friendly:;
		case playtype::prepare_kickoff_enemy:;
		case playtype::execute_kickoff_enemy:;
		case playtype::prepare_penalty_friendly:;
		case playtype::execute_penalty_friendly:;
		case playtype::prepare_penalty_enemy:;
		case playtype::execute_penalty_enemy:;
		case playtype::execute_direct_free_kick_friendly:;
		case playtype::execute_indirect_free_kick_friendly:;
		case playtype::execute_direct_free_kick_enemy:;
		case playtype::execute_indirect_free_kick_enemy:;
		case playtype::pit_stop:;
		case playtype::victory_dance:;
		default:;
		}
	}

	void chase_strategy::set_playtype(playtype::playtype t) {
		current_playtype = t;
	}
	
	Gtk::Widget *chase_strategy::get_ui_controls() {
		return 0;
	}

  void chase_strategy::robot_added(void){
  }

  void chase_strategy::robot_removed(unsigned int index, robot::ptr r){
  }

	class chase_strategy_factory : public virtual strategy_factory {
		public:
			chase_strategy_factory();
			virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	chase_strategy_factory::chase_strategy_factory() : strategy_factory("Chase Strategy") {
	}

	strategy::ptr chase_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new chase_strategy(ball, field, team));
		return s;
	}

	chase_strategy_factory factory;

	strategy_factory &chase_strategy::get_factory() {
		return factory;
	}
}

