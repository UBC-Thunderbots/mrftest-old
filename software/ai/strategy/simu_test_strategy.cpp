#include "ai/strategy.h"
#include "ai/navigator/testnavigator.h"
#include "ai/tactic/chase.h"
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

#include <iostream>
//created by Kenneth Lui, last updated 30 Nov 2009.
//This strategy was created to test the simulator.

namespace simu_test{
  
  class simu_test_strategy : public strategy {
  public:
    simu_test_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
    void tick();
    void set_playtype(playtype::playtype t);
    strategy_factory &get_factory();
    Gtk::Widget *get_ui_controls();
    void robot_added(void);
    void robot_removed(unsigned int index, player::ptr r);
    static bool auto_ref_setup;
    static point ball_pos, ball_vel, player_pos;
    
  private:
    //private variables
    static const int WAIT_AT_LEAST_TURN = 5;		// We need this because we don't want to make frequent changes
    static const int DEFAULT_OFF_TO_DEF_DIFF = 1;	// i.e. one more offender than defender
    int test_id;
    bool test_done;
    bool test_started;
    bool is_ball_in_bound();
    bool is_player_in_pos(player::ptr , double , double);
    bool is_ball_in_pos(double , double);
    navigator::ptr our_navigator;
    player::ptr the_only_player;
  };
  
  //initialization of static variable
  bool simu_test_strategy::auto_ref_setup = true;
  point simu_test_strategy::ball_pos, simu_test_strategy::ball_vel, simu_test_strategy::player_pos;;

  simu_test_strategy::simu_test_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : strategy(ball, field, team, pt_src) {
    // Initialize variables here (e.g. create the roles).
    test_id = 0;
    //simu_test_strategy::
    auto_ref_setup = true;
    test_done = true;
    test_started = false;
    return;
    // problems: how do we keep track of roles?
  }

  bool simu_test_strategy::is_ball_in_bound() {
	if (the_ball->position().x > the_field->length()/2)
		return false;
	if (the_ball->position().x < the_field->length()/-2)
		return false;
	if (the_ball->position().y > the_field->width()/2)
		return false;
	if (the_ball->position().y < the_field->width()/-2)
		return false;
	return true;
  }

  bool simu_test_strategy::is_player_in_pos(player::ptr plr, double x, double y) {
	if (plr->position().x > x+0.05)
		return false;
	if (plr->position().x < x-0.05)
		return false;
	if (plr->position().y > y+0.05)
		return false;
	if (plr->position().y < y-0.05)
		return false;
	return true;
  }

  bool simu_test_strategy::is_ball_in_pos(double x, double y) {
	if (the_ball->position().x > x+0.15)	//0.15 instead of 0.05...because ball enters the area after the player
		return false;
	if (the_ball->position().x < x-0.15)
		return false;
	if (the_ball->position().y > y+0.15)
		return false;
	if (the_ball->position().y < y-0.15)
		return false;
	return true;
  }
  
  void simu_test_strategy::tick() {
	if (pt_source.current_playtype()!=playtype::play)
	{
		return;	//playtype must be play
	}
    // Use the variables "the_ball", "the_field", and "the_team" to allocate players to roles.
	if ((!auto_ref_setup)&&(!test_started))	//now the strategy starts the test
	{
		the_only_player = the_team->get_player(0);
		std::cout << "Test#" << test_id+1 << " Started" << std::endl;
		point move_to_point; 
		switch (test_id)
		{
			case 0: 	//kick
				the_only_player->kick(1);
				std::cout << "Kick - Kick Executed" << std::endl;
				break;
			case 1:	//chip
				the_only_player->chip(1);
				std::cout << "Chip - Chip Executed" << std::endl;
				break;
			case 2:	//move
				our_navigator = navigator::ptr(new testnavigator(the_only_player,the_field,the_ball,the_team));
				move_to_point.x = 2.0;
				move_to_point.y = 0.0;
				our_navigator->set_point(move_to_point);
				our_navigator->tick();
				std::cout << "Move - Move Executed" << std::endl;
				break;
			case 3:		//ball collides with player
					//no action?
				std::cout << "Collide - No action" << std::endl;
				break;
			case 4:		//dribble
				if (the_only_player->has_ball())
				{	std::cout << "Dribble - Has Ball" << std::endl;
				}else
				{	std::cout << "Dribble - Doesn't Has Ball" << std::endl;
				}
				the_only_player->dribble(1);
				std::cout << "Dribble - Dribble Executed" << std::endl;
				our_navigator = navigator::ptr(new testnavigator(the_only_player,the_field,the_ball,the_team));
				move_to_point.x = -2.0;
				move_to_point.y = 0.0;
				our_navigator->set_point(move_to_point);
				our_navigator->tick();
				std::cout << "Dribble - Move Executed" << std::endl;
				break;
			case 5:		//receive	//not implemented yet
				break;
		}
		test_started = true;
		return;		
	}
	if ((test_started)&&(!test_done))
	{
		//check if the action is done; otherwise continue tick() for the lower level;
		switch (test_id)
		{
			case 0: 	//kick
				if (!is_ball_in_bound())
				{
					std::cout << "Ball is out of bound" << std::endl;
					std::cout << "Test#1 Completed" << std::endl;
					test_done = true;
					test_started = false;
					test_id++;
				}else
				{
					if ((the_ball->est_velocity().x >0.0)||(the_ball->est_velocity().y >0.0))
					{
						std::cout << "Ball is still in bound - Moving" << std::endl;
					}else
					{
						std::cout << "Ball is still in bound - Stopped" << std::endl;
					}
				}
				break;
			case 1:	//chip
				if (!is_ball_in_bound())
				{
					std::cout << "Ball is out of bound" << std::endl;
					std::cout << "Test#1 Completed" << std::endl;
					test_done = true;
					test_started = false;
					test_id++;
				}else
				{
					std::cout << "Ball is still in bound" << std::endl;
				}
				break;
			case 2:	//move
				if (is_player_in_pos(the_only_player,2.0,0.0))
				{
					std::cout << "Player arrived at position" << std::endl;
					std::cout << "Test#3 Completed" << std::endl;
					test_done = true;
					test_started = false;
					test_id++;
				}else
				{
					std::cout << "Player is still moving" << std::endl;
					our_navigator->tick();
				}
				break;
			case 3:		//ball collides with player
				if (the_ball->est_velocity().x>0)
				{
					std::cout << "Ball and Player Collided" << std::endl;
					std::cout << "Test#4 Completed" << std::endl;
					test_done = true;
					test_started = false;
					test_id++;
				}else
				{
					std::cout << "Ball is still moving to player" << std::endl;
					our_navigator->tick();
				}
				break;
			case 4:		//dribble
				if (is_player_in_pos(the_only_player,-2.0,0.0))
				{
					std::cout << "Player arrived at position" << std::endl;
					if (is_ball_in_pos(-2.0, 0.0))
					{	std::cout << "Test#5 Completed" << std::endl;
					}else
					{	std::cout << "Test#5 Failed" << std::endl;
					}
					test_done = true;
					test_started = false;
					test_id++;
				}else
				{
					std::cout << "Player is still moving" << std::endl;
					our_navigator->tick();
				}
				break;
			case 5:		//receive	//not implemented yet
				break;
		}
	}
    if ((auto_ref_setup)&&(test_done))
	{
		std::cout << "Test#" << test_id+1 << " Initialization" << std::endl;
		switch (test_id)
		{
			case 0: //kick
			case 1:	//chip
			case 2:	//move
				ball_pos.x = 0.0;
				ball_pos.y = 0.0;
				ball_vel.x = 0.0;
				ball_vel.y = 0.0;
				player_pos.x = -0.05;
				player_pos.y = 0.0;
				break;
			case 3:		//ball collides with player
				ball_pos.x = 1.0;
				ball_pos.y = 0.0;
				ball_vel.x = -1.0;
				ball_vel.y = 0.0;
				player_pos.x = 0.0;
				player_pos.y = 0.0;
				break;
			case 4:		//dribble
				ball_pos.x = 0.0;
				ball_pos.y = 0.0;
				ball_vel.x = 0.0;
				ball_vel.y = 0.0;
				player_pos.x = -0.05;
				player_pos.y = 0.0;
				break;
			case 5:		//receive
				ball_pos.x = 2.0;
				ball_pos.y = 0.0;
				ball_vel.x = -1.0;
				ball_vel.y = 0.0;
				player_pos.x = 0.0;
				player_pos.y = 0.0;
				break;
		}
		test_done = false;
		auto_ref_setup = false; //should let auto ref do this
	}

    
    return;
  }

  void simu_test_strategy::set_playtype(playtype::playtype) { 

  }
  
  Gtk::Widget *simu_test_strategy::get_ui_controls() {
    return 0;
	}
  
	void simu_test_strategy::robot_added(void) {
	  std::cout << "<<<<<<<<<ROBOT ADDED>>>>" << std::endl;	 
	}

	void simu_test_strategy::robot_removed(unsigned int, player::ptr) {	  
	  std::cout << "<<<<<<<<<ROBOT Removed>>>>" << std::endl;
	}

	class simu_test_strategy_factory : public strategy_factory {
		public:
			simu_test_strategy_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);
	};

	simu_test_strategy_factory::simu_test_strategy_factory() : strategy_factory("Simulator Test (For Jason)") {
	}

	strategy::ptr simu_test_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) {
		strategy::ptr s(new simu_test_strategy(ball, field, team, pt_src));
		return s;
	}

	simu_test_strategy_factory factory;

	strategy_factory &simu_test_strategy::get_factory() {
		return factory;
	}
}

