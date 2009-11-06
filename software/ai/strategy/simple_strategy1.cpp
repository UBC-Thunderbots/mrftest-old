#include "ai/role.h"
#include "ai/strategy.h"
#include <map>
#include <set>
//simple strategy created by Armand

namespace {
	class simple_strategy1 : public strategy {
		public:
			simple_strategy1(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, robot::ptr r);

		private:
			playtype::playtype current_playtype;

			// saving the initialized roles for each robot
			std::map< role::ptr, std::set<robot::ptr> > RobotAssignment_;
	};

	simple_strategy1::simple_strategy1(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
		// Initialize variables here (e.g. create the roles).
	}

	void simple_strategy1::tick() {
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

	void simple_strategy1::set_playtype(playtype::playtype t) {
		current_playtype = t;
	}
	
	Gtk::Widget *simple_strategy1::get_ui_controls() {
		return 0;
	}

  void simple_strategy1::robot_added(void){
  }

  void simple_strategy1::robot_removed(unsigned int index, robot::ptr r){
  }

	class simple_strategy1_factory : public strategy_factory {
		public:
			simple_strategy1_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	simple_strategy1_factory::simple_strategy1_factory() : strategy_factory("Simple Strategy 1") {
	}

	strategy::ptr simple_strategy1_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new simple_strategy1(ball, field, team));
		return s;
	}

	simple_strategy1_factory factory;

	strategy_factory &simple_strategy1::get_factory() {
		return factory;
	}
}

