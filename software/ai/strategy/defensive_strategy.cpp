// Some stuff added by Terence (mainly just copied from offensive_strategy), needs more implementation

#include "ai/strategy/strategy.h"
#include "ai/role/role.h"

#include "ai/role/defensive.h"
#include "ai/role/goalie.h"
#include "ai/role/execute_direct_free_kick_enemy.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_direct_free_kick_friendly.h" 
#include "ai/role/pit_stop.h"
#include "ai/role/execute_indirect_free_kick_enemy.h"     
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/execute_indirect_free_kick_friendly.h"  
#include "ai/role/prepare_kickoff_friendly.h"              
#include "ai/role/prepare_penalty_enemy.h"
#include "ai/role/execute_kickoff_friendly.h"             
#include "ai/role/prepare_penalty_friendly.h"
#include "ai/role/execute_penalty_enemy.h"
#include "ai/role/victory_dance.h"
#include "ai/role/execute_penalty_friendly.h"

#include <iostream> // for debugging purposes
using namespace std; 

namespace {

	// Check the number of Enemies on our field
	// pass in your own team and call other to get the other team
	int NumOfEnemiesOnOurField(world::ptr world){		
		int cnt = 0;
		for (unsigned int i = 0 ; i < world->enemy.size() ; i++){
			if (world->enemy.get_robot(i)->position().x < 0) cnt++;
		}
		return cnt;
	}
	// return the dist of robot to ball
	double distToBall(robot::ptr robot, ball::ptr ball){
		return sqrt(pow(robot->position().x - ball->position().x, 2) +  pow(robot->position().y - ball->position().y, 2));
	}
	// return the dist of robot to goal, pass goal == 0 when it's your own goal, goal == 1 when it's the other goal
	double distToGoal(robot::ptr robot, const field &field, int goal){
		double dist = 0.0;
		if (goal == 1)
			dist = sqrt(pow(field.length()/2 - robot->position().x, 2) +  pow(robot->position().y, 2));
		else 
			dist = sqrt(pow(robot->position().x - field.length()/2, 2) +  pow(robot->position().y, 2));
		return dist;
	}
	// return dist of one robot to another, can't think of any practical use of this now, but meh.
	double distToRobot(robot::ptr robot1, robot::ptr robot2){
		return sqrt(pow(robot1->position().x - robot2->position().x, 2) +  pow(robot1->position().y - robot2->position().y, 2));
	}
	// return index of closest robot to ball
	unsigned int closestRobotToBall(const team &team, ball::ptr ball){
		int closest = 0;
		for (unsigned int i = 1 ; i < team.size() ; i++){
			if (distToBall(team.get_robot(closest), ball) > distToBall(team.get_robot(i), ball)) closest = i;
		}
		return closest;
	}
	// return index of closest robot to goal, pass goal == 0 when it's your own goal, goal == 1 when it's the other goal
	unsigned int closestRobotToGoal(const team &team, const field &field, int goal){
		int closest = 0;
		for (unsigned int i = 1 ; i < team.size() ; i++){
			if (distToGoal(team.get_robot(closest), field, goal) > distToGoal(team.get_robot(i), field, goal)) closest = i;
		}
		return closest;
	}
	// return index of closest robot to another robot
	unsigned int closestRobotToRobot(const team &team , robot::ptr robot){
		int closest = 0;
		for (unsigned int i = 1 ; i < team.size() ; i++){
			if (distToRobot(robot,team.get_robot(closest)) > distToRobot(robot,team.get_robot(i)) 
			    && distToRobot(robot,team.get_robot(i)) != 0.0) closest = i;
		}
		return closest;
	}



	class defensive_strategy : public strategy {
		public:
			defensive_strategy(world::ptr world);
			void tick();
			void set_playtype(playtype::playtype t);
			const world::ptr the_world;
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();

		private:

			// Create variables here (e.g. to store the roles).
			vector<role::ptr> roles;
			playtype::playtype current_playtype;
	};

	defensive_strategy::defensive_strategy(world::ptr world) : the_world(world) {
		// Initialize variables here (e.g. create the roles).

		// Initialize: everybody a happy defender :)
		for (unsigned int i = 0 ; i < the_world->friendly.size() ; i++){ 
		 	roles.push_back(role::ptr(new defensive(the_world))); 
		}
	}

	void defensive_strategy::tick() {
		// Use the variables "ball", "field", and "team" to allocate players to roles.

		switch (current_playtype)
		{
			case playtype::play:	for (unsigned int i = 0 ; i < roles.size() ; i++){
							roles[i]->tick();
						}
						break;
			default	:		break;
		}
	}

	void defensive_strategy::set_playtype(playtype::playtype pt) {
		current_playtype = pt;
	}
	
	Gtk::Widget *defensive_strategy::get_ui_controls() {
		return 0;
	}

	class defensive_strategy_factory : public strategy_factory {
		public:
			defensive_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	defensive_strategy_factory::defensive_strategy_factory() : strategy_factory("defensive Strategy") {
	}

	strategy::ptr defensive_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new defensive_strategy(world));
		return s;
	}

	defensive_strategy_factory factory;

	strategy_factory &defensive_strategy::get_factory() {
		return factory;
	}
}
