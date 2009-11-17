#include "ai/role.h"
#include "ai/strategy.h"
#include "ai/role/goalie.h"
#include "ai/role/pit_stop.h"
#include <map>
#include <set>
//simple strategy created by Armand

#define all(container, it) \
	typeof(container.begin()) it = container.begin(); it != container.end(); ++it

namespace {
	class simple_strategy1 : public strategy {
		public:
			simple_strategy1(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, robot::ptr r);

		private:
			
			// private methods
			void getAllRobots();
			void clearRoles();

			// saving the robot IDs mapped to its role
			std::map< robot::ptr, role::ptr > assignments_;
			std::set< role::ptr > existingRoles_;
	};

	simple_strategy1::simple_strategy1(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src) {
		// Get all robots currently assigned to the team
		getAllRobots();
		/* Assign the robots to their respective roles, if playtype has already
		 * been set
		 */
		tick();
	}

	void simple_strategy1::tick() {
		// Use the variables "ball", "field", and "team" to allocate players to roles.

		switch (pt_source.current_playtype()) {
		case playtype::halt:
			clearRoles();
			break;
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

	void simple_strategy1::getAllRobots(){
		for (unsigned int i = 0; i < the_team->size(); ++i){
			robot::ptr rp = the_team->get_robot(i);
			if (assignments_.find(rp) != assignments_.end()){
				assignments_[rp] = pit_stop::ptr();
			}
		}
	}

	void simple_strategy1::clearRoles(){
		existingRoles_.clear();
		assignments_.clear();
		getAllRobots();
	}

	void simple_strategy1::set_playtype(playtype::playtype) {
		tick();
	}
	
	Gtk::Widget *simple_strategy1::get_ui_controls() {
		return 0;
	}

	void simple_strategy1::robot_added(void){
		for (unsigned int i = 0; i < the_team->size(); ++i){
			robot::ptr rp = the_team->get_robot(i);
			if (assignments_.find(rp) != assignments_.end()){
				assignments_[rp] = pit_stop::ptr();
			}
		}
		tick();
	}

	void simple_strategy1::robot_removed(unsigned int index, robot::ptr r){
		assignments_.erase(r);
		tick();
	}

	class simple_strategy1_factory : public strategy_factory {
		public:
			simple_strategy1_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
	};

	simple_strategy1_factory::simple_strategy1_factory() : strategy_factory("Simple Strategy 1") {
	}

	strategy::ptr simple_strategy1_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) {
		strategy::ptr s(new simple_strategy1(ball, field, team, pt_src));
		return s;
	}

	simple_strategy1_factory factory;

	strategy_factory &simple_strategy1::get_factory() {
		return factory;
	}
}

