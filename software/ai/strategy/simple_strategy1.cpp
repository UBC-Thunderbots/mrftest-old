#include "ai/role/role.h"
#include "ai/strategy/strategy.h"
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
			simple_strategy1(world::ptr world);
			void tick();
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, player::ptr r);

		private:
			
			// private methods
			void getAllRobots();
			void clearRoles();

			const world::ptr the_world;

			// saving the robot IDs mapped to its role
			std::map< robot::ptr, role::ptr > assignments_;
			std::set< role::ptr > existingRoles_;
	};

	simple_strategy1::simple_strategy1(world::ptr world) : the_world(world) {
		// Connect to the team change signals.
		world->friendly.signal_player_added.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &simple_strategy1::robot_added))));
		world->friendly.signal_player_removed.connect(sigc::mem_fun(this, &simple_strategy1::robot_removed));
		// Get all robots currently assigned to the team
		getAllRobots();
		/* Assign the robots to their respective roles, if playtype has already
		 * been set
		 */
		tick();
	}

	void simple_strategy1::tick() {
		// Use the variables "ball", "field", and "team" to allocate players to roles.

		switch (the_world->playtype()) {
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
		const team &the_team(the_world->friendly);
		for (unsigned int i = 0; i < the_team.size(); ++i){
			robot::ptr rp = the_team.get_robot(i);
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

	Gtk::Widget *simple_strategy1::get_ui_controls() {
		return 0;
	}

	void simple_strategy1::robot_added(void){
		const team &the_team(the_world->friendly);
		for (unsigned int i = 0; i < the_team.size(); ++i){
			robot::ptr rp = the_team.get_robot(i);
			if (assignments_.find(rp) != assignments_.end()){
				assignments_[rp] = pit_stop::ptr();
			}
		}
		tick();
	}

	void simple_strategy1::robot_removed(unsigned int, player::ptr r){
		assignments_.erase(robot::ptr::cast_static(r));
		tick();
	}

	class simple_strategy1_factory : public strategy_factory {
		public:
			simple_strategy1_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	simple_strategy1_factory::simple_strategy1_factory() : strategy_factory("Simple Strategy 1") {
	}

	strategy::ptr simple_strategy1_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new simple_strategy1(world));
		return s;
	}

	simple_strategy1_factory factory;

	strategy_factory &simple_strategy1::get_factory() {
		return factory;
	}
}

