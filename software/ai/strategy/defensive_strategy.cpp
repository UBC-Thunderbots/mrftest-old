// Some stuff added by Terence (mainly just copied from offensive_strategy), needs more implementation

#include "ai/strategy.h"
#include "ai/role.h"

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
#include "ai/role/execute_kickoff_enemy.h"                
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
	int NumOfEnemiesOnOurField(controlled_team::ptr team){		
		int cnt = 0;
		for (unsigned int i = 0 ; i < team->other()->size() ; i++){
			if (team->other()->get_robot(i)->position().x < 0) cnt++;
		}
		return cnt;
	}
	// return the dist of robot to ball
	double distToBall(robot::ptr robot, ball::ptr ball){
		return sqrt(pow(robot->position().x - ball->position().x, 2) +  pow(robot->position().y - ball->position().y, 2));
	}
	// return the dist of robot to goal, pass goal == 0 when it's your own goal, goal == 1 when it's the other goal
	double distToGoal(robot::ptr robot, field::ptr field, int goal){
		double dist = 0.0;
		if (goal == 1)
			dist = sqrt(pow(field->length()/2 - robot->position().x, 2) +  pow(robot->position().y, 2));
		else 
			dist = sqrt(pow(robot->position().x - field->length()/2, 2) +  pow(robot->position().y, 2));
		return dist;
	}
	// return dist of one robot to another, can't think of any practical use of this now, but meh.
	double distToRobot(robot::ptr robot1, robot::ptr robot2){
		return sqrt(pow(robot1->position().x - robot2->position().x, 2) +  pow(robot1->position().y - robot2->position().y, 2));
	}
	// return index of closest robot to ball
	unsigned int closestRobotToBall(controlled_team::ptr team, ball::ptr ball){
		int closest = 0;
		for (unsigned int i = 1 ; i < team->size() ; i++){
			if (distToBall(team->get_robot(closest), ball) > distToBall(team->get_robot(i), ball)) closest = i;
		}
		return closest;
	}
	// return index of closest robot to goal, pass goal == 0 when it's your own goal, goal == 1 when it's the other goal
	unsigned int closestRobotToGoal(controlled_team::ptr team, field::ptr field, int goal){
		int closest = 0;
		for (unsigned int i = 1 ; i < team->size() ; i++){
			if (distToGoal(team->get_robot(closest), field, goal) > distToGoal(team->get_robot(i), field, goal)) closest = i;
		}
		return closest;
	}
	// return index of closest robot to another robot
	unsigned int closestRobotToRobot(controlled_team::ptr team , robot::ptr robot){
		int closest = 0;
		for (unsigned int i = 1 ; i < team->size() ; i++){
			if (distToRobot(robot,team->get_robot(closest)) > distToRobot(robot,team->get_robot(i)) 
			    && distToRobot(robot,team->get_robot(i)) != 0.0) closest = i;
		}
		return closest;
	}



	class defensive_strategy : public strategy {
		public:
			defensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_added(void);
			void robot_removed(unsigned int index, player::ptr r);

		private:

			// Create variables here (e.g. to store the roles).
			vector<role::ptr> roles;
			playtype::playtype current_playtype;
	};

	defensive_strategy::defensive_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src) {
		// Initialize variables here (e.g. create the roles).

		// Initialize: everybody a happy defender :)
		for (unsigned int i = 0 ; i < team->size() ; i++){ 
		 	roles.push_back(role::ptr(new defensive(ball, field, team))); 
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

  void defensive_strategy::robot_added(void){
  }

  void defensive_strategy::robot_removed(unsigned int, player::ptr){
  }

	class defensive_strategy_factory : public strategy_factory {
		public:
			defensive_strategy_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
	};

	defensive_strategy_factory::defensive_strategy_factory() : strategy_factory("defensive Strategy") {
	}

	strategy::ptr defensive_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) {
		strategy::ptr s(new defensive_strategy(ball, field, team, pt_src));
		return s;
	}

	defensive_strategy_factory factory;

	strategy_factory &defensive_strategy::get_factory() {
		return factory;
	}
}
