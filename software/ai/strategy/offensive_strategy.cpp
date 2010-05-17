// made by Terence under braindead conditions with a crazy mind, needs more implementation
// much thanks to Kenneth since I used a lot of his code as reference

#include "ai/role/role.h"
#include "ai/strategy/strategy.h"

#include "ai/role/defensive.h"
#include "ai/role/goalie.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_direct_free_kick_friendly.h" 
#include "ai/role/pit_stop.h"     
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/execute_indirect_free_kick_friendly.h"  
#include "ai/role/prepare_kickoff_friendly.h"               
#include "ai/role/prepare_penalty_enemy.h"
#include "ai/role/execute_kickoff_friendly.h"             
#include "ai/role/prepare_penalty_friendly.h"
#include "ai/role/execute_penalty_enemy.h"
#include "ai/role/victory_dance.h"
#include "ai/role/execute_penalty_friendly.h"

#include <iostream>  // for debugging purposes
using namespace std; 

namespace {
	
	// struct to store the details of robot, NOTE: my robot_details have different members than Kenneth's robot_details
	struct robot_details{
    		int index;	// index in team
   	 	double dist_to_ball;		
		double dist_to_goal;  // default goal your own goal
		double dist_to_robot; // default robot should be goalie, 
				      // but you can always update this to have the distance of this robot to another particular robot
  	};

	// some comparators

	bool dist_to_ball_cmp ( const robot_details &a, const robot_details &b)		// first ele is the one having smallest x
  	{
    		return a.dist_to_ball < b.dist_to_ball;
  	}
  
  	bool dist_to_goal_cmp ( const robot_details &a, const robot_details &b)		// first ele is the one having smallest dist to ball
  	{
    		return a.dist_to_goal < b.dist_to_goal;
  	}

	bool dist_to_robot_cmp ( const robot_details &a, const robot_details &b)		// first ele is the one having smallest x
  	{
    		return a.dist_to_robot < b.dist_to_robot;
  	}

	// some helpers

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
	

	class offensive_strategy : public strategy {
		public:
			offensive_strategy(world::ptr world);
			void tick();
			void set_playtype(playtype::playtype t);
			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();
			void robot_removed(unsigned int index, player::ptr r);

		private:

			// Create variables here (e.g. to store the roles).
			const world::ptr the_world;
			vector<role::ptr> roles;
			playtype::playtype current_playtype;
			vector<robot_details> robot_det;
			vector<int> closestPlayersToBall;  // use to keep track the closest players to the ball
			vector<int> closestPlayersToGoal;  // use to keep track the closest players to the goal, default goal your own goal
			vector<int> closestPlayersToRobot; // use to keep track the closest players to a robot, 
							   // default robot will be the one closest to goal
	};

	offensive_strategy::offensive_strategy(world::ptr world) : the_world(world) {
		const friendly_team &team(the_world->friendly);
		const ball::ptr ball(the_world->ball());
		const field &field(the_world->field());
		// Initialize variables here (e.g. create the roles).
		
		// Initialize: everybody a happy offender :)
		for (unsigned int i = 0 ; i < team.size() ; i++){ 
		 	roles.push_back(role::ptr(new offensive(the_world))); 
		}   			  	
		
		// Initialize robot_det
		for (unsigned int i = 0 ; i < team.size() ; i++){
			robot_details rd;
			rd.index = i;
			rd.dist_to_ball = distToBall(team.get_robot(i), ball);
			rd.dist_to_goal = distToGoal(team.get_robot(i), field, 0);
			rd.dist_to_robot = distToRobot(team.get_robot(i), team.get_robot(closestRobotToGoal(team, field, 0)));
			robot_det.push_back(rd);
		}
	}

	void offensive_strategy::tick() {
		const friendly_team &the_team(the_world->friendly);
		const ball::ptr the_ball(the_world->ball());
		const field &the_field(the_world->field());

		// There should be a switch statement to take care of the different playtypes
		// the implementation below is just for the "play" playtype
		// also the implementation for setting role goalie may be wrong

		switch (current_playtype)
		{
			case playtype::play:	for (unsigned int i = 0 ; i < roles.size() ; i++){
							roles[i]->tick();
						}
						break;
			default	:		break;
		}

		// Use the variables "the_ball", "the_field", and "the_team" to allocate players to roles.
		
		// calls role::update

		
		// update the details of the robot for every tick
		robot_det.clear();
		for (unsigned int i = 0 ; i < the_team.size() ; i++){
			robot_details rd;
			rd.index = i;			
			rd.dist_to_ball = distToBall(the_team.get_player(i), the_ball);
			rd.dist_to_goal = distToGoal(the_team.get_player(i), the_field, 0);
			rd.dist_to_robot = distToRobot(the_team.get_player(i), the_team.get_player(closestRobotToGoal(the_team, the_field, 0)));
			robot_det.push_back(rd);			
		}

		// should update the 3 vectors closestPlayersToBall, closestPlayersToGoal, closestPlayersToRobot for every tick
		// std::sort(closestPlayersToBall.begin(), closestPlayersToBall.end() , dist_to_ball_cmp);

		// code beyond this point doesn't have all the functionalities declared, needs more implementations

		// ultimate offensive strategy: all players switch to offenders (including goalie, mwahahahahahaha)
		// fires once the ball gets to the other side 
		if (the_ball->position().x > 0) {
			for (unsigned int i = 0 ; i < the_team.size() ; i++) {
				// set every player to offenders, mwahahaha 
				roles[i] = role::ptr(new offensive(the_world));
			}
		}

		
		// else if the enemy got more than 3 robots flooding our side of the field, PANIC, 2 offender, 2 defender, and 1 goalie?
		// if you want to have a stronger defence, change to defensive strategy, I believe this is the best defensive mode for offensive strategy
		else if (NumOfEnemiesOnOurField(the_world) >= 3){
			// set closest player to own goal to goalie 
			for (unsigned int i = 0 ; i < the_team.size() ; i++){
				if (i == closestRobotToGoal(the_team, the_field, 0)){
					roles[i] = role::ptr(new goalie(the_world));
				}
			}
			// set next two closest players to goalie to defender, broken	 
			for (unsigned int i = 0 ; i < the_team.size() ; i++){
				if (i == closestRobotToRobot(the_team, the_team.get_player(closestRobotToGoal(the_team, the_field, 0)))){
					roles[i] = role::ptr(new defensive(the_world));
				}
			}			
		}
		
		else {			
			// set closest player to ball to offender
			for (unsigned int i = 0 ; i < the_team.size() ; i++){
				if (i == closestRobotToBall(the_team, the_ball)){
					roles[i] = role::ptr(new offensive(the_world));
				}
			}
			// set closest player to own goal to goalie
			for (unsigned int i = 0 ; i < the_team.size() ; i++){
				if (i == closestRobotToGoal(the_team, the_field, 0)){
					roles[i] = role::ptr(new goalie(the_world));
				}
			}
			// set closest player to goalie as defender
			for (unsigned int i = 0 ; i < the_team.size() ; i++){
				if (i == closestRobotToRobot(the_team, the_team.get_player(closestRobotToGoal(the_team, the_field, 0)))){
					roles[i] = role::ptr(new defensive(the_world));
				}
			}
			
		}
		
	}

	void offensive_strategy::set_playtype(playtype::playtype pt) {
		current_playtype = pt;
	}
	
	Gtk::Widget *offensive_strategy::get_ui_controls() {
		return 0;
	}

  void offensive_strategy::robot_removed(unsigned int, player::ptr){
  }

	class offensive_strategy_factory : public strategy_factory {
		public:
			offensive_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	offensive_strategy_factory::offensive_strategy_factory() : strategy_factory("Offensive Strategy") {
	}

	strategy::ptr offensive_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new offensive_strategy(world));
		return s;
	}

	offensive_strategy_factory factory;

	strategy_factory &offensive_strategy::get_factory() {
		return factory;
	}
}

