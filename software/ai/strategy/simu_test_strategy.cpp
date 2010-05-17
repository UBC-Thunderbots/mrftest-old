#include "ai/strategy/strategy.h"
#include "ai/navigator/testnavigator.h"
#include "ai/tactic/chase.h"
#include "ai/role/role.h"
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
#include "simu_test_strategy.h"
#include <iostream>
//created by Kenneth Lui, last updated 2 Dec 2009.
//This strategy was created to test the simulator.

namespace simu_test{
  

  //initialization of static variable
 // static bool simu_test_strategy::auto_ref_setup = true;
 
  bool simu_test_strategy::auto_ref_setup = true;
 point simu_test_strategy::ball_pos(0,0), simu_test_strategy::ball_vel(0,0), simu_test_strategy::player_pos(0,0);
 

//auto_ref_setup = true;


  simu_test_strategy::simu_test_strategy(world::ptr world) : the_world(world) {
	friendly_team &team(the_world->friendly);
    // Connect to the team change signals.
    team.signal_player_added.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &simu_test_strategy::robot_added))));
	team.signal_player_removed.connect(sigc::mem_fun(this, &simu_test_strategy::robot_removed));
    // Initialize variables here (e.g. create the roles).
    test_id = 0;
    auto_ref_setup = true;
    test_done = true;
    test_started = false;
    tc_receive_receiving = false;
    tests_completed = false;
    first_tick = true;
    tc_receive_receive_count = 0;
    print_msg = true;
    print_msg2 = true;
    tick_count = 0;
    for (int i = 0; i<6; i++){result[i] = false;}
    return;
    // problems: how do we keep track of roles?
  }

  bool simu_test_strategy::is_ball_in_bound() {
	const ball::ptr the_ball(the_world->ball());
	const field &the_field(the_world->field());
    if (the_ball->position().x > the_field.length()/2)
      return false;
    if (the_ball->position().x < the_field.length()/-2)
      return false;
    if (the_ball->position().y > the_field.width()/2)
      return false;
    if (the_ball->position().y < the_field.width()/-2)
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
	const ball::ptr the_ball(the_world->ball());
    if (the_ball->position().x > x+0.25)	//0.25 instead of 0.05...because ball enters the area after the player
      return false;
    if (the_ball->position().x < x-0.25)
      return false;
    if (the_ball->position().y > y+0.25)
      return false;
    if (the_ball->position().y < y-0.25)
      return false;
    return true;
  }

  void simu_test_strategy::finish_test_case()
  {
    test_done = true;
    test_started = false;
    test_id++;
    print_msg = true;
    print_msg2 = true;
  }
  
  void simu_test_strategy::tick() {
	const ball::ptr the_ball(the_world->ball());
	const friendly_team &the_team(the_world->friendly);
    tick_count++;
    if (!the_team.size()) return;
    point move_to_point; 
    the_only_player = the_team.get_player(0);
    //    the_only_player->dribble(1);
    if (first_tick)
      {
	stay_here = the_only_player->position();
	our_navigator = new testnavigator(the_only_player,the_world);
	first_tick = false;
      }
    if ((stay_here - the_only_player->position()).len() > 0.3)
      {
	std::cout << "moved" << std::endl;
	stay_here = the_only_player->position();
      }
    else if ((stay_here - the_only_player->position()).len() < 0.2)
	  { // return;// don't move
	  }
    our_navigator->set_point(stay_here);
    our_navigator->tick();	
    //    if (pt_source.current_playtype()!=playtype::play)
    //	{}
	  // our_navigator = navigator::ptr(new testnavigator(the_only_player,the_field,the_ball,the_team));
	  // move_to_point = the_only_player->position()  //this doesn't work
	  //  std::cout << "moving to curr. pos.  x,y"<< stay_here.x << " ";
	  //  std::cout << "," << stay_here.y << std::endl;
	  // anyway to stop the god damn motor?
    //    return;	//playtype must be play to do the test
    if (test_id>5)
      {
	if (!tests_completed)
	  {
	    std::cout << "Test Completed" << std::endl;
	    std::cout << "Test Result:" << std::endl;
	    for (int i=0;i<6;i++)
	      {
		std::cout << "Test #" << i+1 <<": " << (result[i] ? "Passed" : "Failed") << std::endl;		
	      }
	    tests_completed = true;
	  }
	return;
      }
    // Use the variables "the_ball", "the_field", and "the_team" to allocate players to roles.
    if ((!auto_ref_setup)&&(!test_started))	//now the strategy starts the test
	{
	  if (print_msg)
	    {
	      std::cout << "Test#" << test_id+1 << " Started" << std::endl;
	      print_msg = false;
	    }
	  switch (test_id)
	    {
	    case 0: 	//kick
	      if (the_only_player->has_ball())
		{ the_only_player->kick(1);
		  std::cout << "Kick - Kick Executed" << std::endl;
		  test_started = true;  
		}
	      break;
	    case 1:	//chip
	      if (the_only_player->has_ball())
		{ the_only_player->chip(1);
		  std::cout << "Chip - Chip Executed" << std::endl;
		  test_started = true;  
		}
	      break;
	    case 2:	//move
	      //	      our_navigator = navigator::ptr(new testnavigator(the_only_player,the_field,the_ball,the_team));
	      move_to_point.x = 2.0;
	      move_to_point.y = 0.0;
	      our_navigator->set_point(move_to_point);
	      our_navigator->tick();
	      std::cout << "Move - Move Executed" << std::endl;
	      test_started = true;  	      
	      break;
	    case 3:		//ball collides with player
	      //no action?
	      std::cout << "Collide - No action" << std::endl;
	      test_started = true;  
	      break;
	    case 4:		//dribble
	      if (the_only_player->has_ball())
		{ std::cout << "Dribble - Has Ball" << std::endl;
		  test_started = true;  
		  the_only_player->dribble(1);
		  std::cout << "Dribble - Dribble Executed" << std::endl;
		  our_navigator = new testnavigator(the_only_player,the_world);
		  move_to_point.x = -2.0;
		  move_to_point.y = 0.0;
		  our_navigator->set_point(move_to_point);
		  our_navigator->tick();
		  std::cout << "Dribble - Move Executed" << std::endl;
		}else
		{ if (print_msg2)
		    {
		      std::cout << "Dribble - Doesn't Has Ball" << std::endl;
		      print_msg2 = false;
		    }
		}
	      break;
	    case 5:		//receive
	      if (the_only_player->has_ball())
		{ std::cout << "Receive - Has Ball" << std::endl;
		  the_only_player->dribble(1);
		  std::cout << "Receive - Dribble Executed" << std::endl;
		  test_started = true;  
		}else
		{ std::cout << "Receive - Doesn't Has Ball" << std::endl;
		}
	      break;
	    }	  
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
		    std::cout << "Test#1 - Ball is out of bound" << std::endl;
		    std::cout << "Test#1 Completed" << std::endl;
		    result[test_id] = true;
		    finish_test_case();
		  }else
		  {
		    if ((the_ball->est_velocity().x >0.0)||(the_ball->est_velocity().y >0.0))
		      {
			if (tick_count%30==0)
			  {
			    std::cout << "Test#1 - Ball is still in bound - Moving" << std::endl;
			    tick_count = 0 ;
			  }
		      }else
		      {
			std::cout << "Test#1 - Ball is still in bound - Stopped" << std::endl;
		      }
		  }
		break;
	      case 1:	//chip
		if (!is_ball_in_bound())
		  {
		    std::cout << "Test#2 Ball is out of bound" << std::endl;
		    std::cout << "Test#2 Completed" << std::endl;
		    result[test_id] = true;
		    finish_test_case();
		  }else
		  {
		    if ((the_ball->est_velocity().x >0.00001)||(the_ball->est_velocity().y >0.00001)||
		    	(the_ball->est_velocity().x <-0.00001)||(the_ball->est_velocity().y <-0.00001))
		      {
			if (tick_count%30==0)
			  {
			    std::cout << "Test#2 - Ball is still in bound - Moving" << std::endl;
			    tick_count = 0;
			  }
		      }else
		      {
			std::cout << "Test#2 - Ball is still in bound - Stopped" << std::endl;
		      }
		  }
		break;
	      case 2:	//move
		if (is_player_in_pos(the_only_player,2.0,0.0))
		  {
		    std::cout << "Test#3 Player arrived at position" << std::endl;
		    std::cout << "Test#3 Completed" << std::endl;
		    result[test_id] = true;
		    finish_test_case();
		  }else
		  {
		    move_to_point.x = 2.0;
		    move_to_point.y = 0.0;
		    if (tick_count%30==0)
		      {
			std::cout << "Test#3 Player is still moving to"<<move_to_point.x<<" " << move_to_point.y << std::endl;
			tick_count =0;
		      }
		    our_navigator->set_point(move_to_point);
		    our_navigator->tick();
		  }
		break;
	      case 3:		//ball collides with player
		if (the_ball->est_velocity().x>0)
		  {
		    std::cout << "Test#4 Ball and Player Collided" << std::endl;
		    std::cout << "Test#4 Completed" << std::endl;
		    result[test_id] = true;
		    finish_test_case();
		  }else
		  {
		    std::cout << "Test#4 Ball is still moving to player" << std::endl;
		    our_navigator->set_point(move_to_point);
		    our_navigator->tick();
		  }
		break;
	      case 4:		//dribble
		if (is_player_in_pos(the_only_player,-2.0,0.0))
		  {
		    std::cout << "Test#5 Player arrived at position" << std::endl;
		    if (is_ball_in_pos(-2.0, 0.0))
		      {	std::cout << "Test#5 Completed" << std::endl;
			result[test_id] = true;
		      }else
		      {	std::cout << "Test#5 Failed" << std::endl;
		      }
		    finish_test_case();
		  }else
		  {
		    if (tick_count%30 == 0)
		      {		       
			if ((the_ball->position() - the_only_player->position()).len() < 0.15)
			  {
			    std::cout << "Test#5 Player is still moving with the ball" << std::endl;
			  }
			else{
			  std::cout << "Test#5 Player is moving without the ball" << std::endl;
			  // finish_test_case();
			}
			tick_count=0;
		      }
		    move_to_point.x = -2.0;
		    move_to_point.y = 0.0;
		    the_only_player->dribble(1);
		    our_navigator->set_point(move_to_point);
		    our_navigator->tick();
		  }
		break;
	      case 5:		//receive
		if (!tc_receive_receiving)
		  {
		    if (the_only_player->has_ball())
		      {
			std::cout << "Test#6 Ball received, wait to see if the ball will bounce off." << std::endl;
			tc_receive_receiving = true;
		      }else
		      {
			std::cout << "Test#6 Waiting for ball." << std::endl;
		      }
		  }else
		  {
		    if (the_only_player->has_ball())
		      {
			tc_receive_receive_count++;
			std::cout << "Test#6 Timestep:" << tc_receive_receive_count << " Still has ball." << std::endl;
			result[test_id] = true;
		      }else
		      {
			tc_receive_receive_count++;
			std::cout << "Test#6 Timestep:" << tc_receive_receive_count << " lost ball." << std::endl;
			result[test_id] = false;
			finish_test_case();// twice...
		      }
		    if (tc_receive_receive_count==100)
		      {
			std::cout << "Test#6 Completed" << std::endl;
			finish_test_case();
		      }
		  }
		break;
	      }
	    //	    return;
	  }
	if (test_done)
	  {
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
		//skip collide, ball can't move.//********
		test_id++;
		return;
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
		//skip receive, ball can't move.//********
		test_id++;
		return;
		break;
	      default: return;
	      }
	    auto_ref_setup = true;
	    std::cout << "Test#" << test_id+1 << " Initialization" << std::endl;
	    test_done = false;
	   // auto_ref_setup = false; //should let auto ref do this
	  }
	return;
  }

  Gtk::Widget *simu_test_strategy::get_ui_controls() {
    return 0;
	}
  
	void simu_test_strategy::robot_added(void) {
	  std::cout << "<<<<<<<<<ROBOT ADDED>>>>" << std::endl;	 
	  if (the_world->friendly.size() == 1) {
		first_tick = true;
	  }
	}

	void simu_test_strategy::robot_removed(unsigned int, player::ptr plr) {	  
	  std::cout << "<<<<<<<<<ROBOT Removed>>>>" << std::endl;
	  if (plr == the_only_player) {
		the_only_player.reset();
		our_navigator = 0;
	  }
	}

	class simu_test_strategy_factory : public strategy_factory {
		public:
			simu_test_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	simu_test_strategy_factory::simu_test_strategy_factory() : strategy_factory("Simulator Test (For Jason)") {
	}

	strategy::ptr simu_test_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new simu_test_strategy(world));
		return s;
	}

	simu_test_strategy_factory factory;

	strategy_factory &simu_test_strategy::get_factory() {
		return factory;
	}
}

