#include "ai/strategy/basic_strategy.h"
#include "ai/tactic/chase.h"
#include "ai/role/defensive.h"
#include "ai/role/goalie.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_free_kick.h" 
#include "ai/role/pit_stop.h"   
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/kickoff_friendly.h"             
#include "ai/role/penalty_friendly.h"
#include "ai/role/penalty_enemy.h"
#include "ai/role/victory_dance.h"


#include <iostream>

namespace {

	class basic_strategy_factory : public strategy_factory {
		public:
			basic_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	basic_strategy_factory::basic_strategy_factory() : strategy_factory("Basic Strategy") {
	}

	strategy::ptr basic_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new basic_strategy(world));
		return s;
	}

	basic_strategy_factory factory;
}

basic_strategy::basic_strategy(world::ptr world) : the_world(world) {
	update_wait = 0;
	update_wait_turns = 5;
	the_world->friendly.signal_robot_added.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &basic_strategy::reset_all))));
	the_world->friendly.signal_robot_removed.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &basic_strategy::reset_all))));
	the_world->signal_playtype_changed.connect(sigc::mem_fun(this, &basic_strategy::reset_all));
}

void basic_strategy::tick() {
	update_wait++;
	if (update_wait >= update_wait_turns) {
		update_wait = 0;
		if (the_world->playtype() == playtype::play)
			in_play_assignment();
	}
	for (size_t i = 0; i < roles.size(); i++) {
		roles[i]->tick();
	}
}

Gtk::Widget *basic_strategy::get_ui_controls() {
	return NULL;
}

void basic_strategy::in_play_assignment() {
	const friendly_team &the_team(the_world->friendly);

	roles.clear();
	if (the_team.size() == 0) return;

	goalie::ptr goalie_role(new goalie(the_world));
	roles.push_back(role::ptr(goalie_role));
	std::vector<player::ptr> goalie;
	goalie.push_back(the_team.get_player(0));
	goalie_role->set_robots(goalie);

	defensive::ptr defensive_role(new defensive(the_world));
	offensive::ptr offensive_role(new offensive(the_world));
	roles.push_back(role::ptr(defensive_role));
	roles.push_back(role::ptr(offensive_role));
	std::vector<player::ptr> defenders;
	std::vector<player::ptr> offenders;

	if (the_team.size() >= 2)
		offenders.push_back(the_team.get_player(1));

	if (the_team.size() >= 3)
		defenders.push_back(the_team.get_player(2));

	if (the_team.size() >= 4)
		offenders.push_back(the_team.get_player(3));

	if (the_team.size() >= 5)
		defenders.push_back(the_team.get_player(4));
	
	// extra players become offenders
	for (size_t i = 5; i < the_team.size(); ++i)
		offenders.push_back(the_team.get_player(i));

	defensive_role->set_robots(defenders);
	offensive_role->set_robots(offenders);
}

player::ptr basic_strategy::minus_one_assignment() {
	const friendly_team &the_team(the_world->friendly);

	roles.clear();
	if (the_team.size() == 0) return player::ptr();

	if (the_team.size() == 1) return the_team.get_player(0);

	goalie::ptr goalie_role(new goalie(the_world));
	roles.push_back(role::ptr(goalie_role));
	std::vector<player::ptr> goalie;
	goalie.push_back(the_team.get_player(0));
	goalie_role->set_robots(goalie);

	defensive::ptr defensive_role(new defensive(the_world));
	offensive::ptr offensive_role(new offensive(the_world));
	roles.push_back(role::ptr(defensive_role));
	roles.push_back(role::ptr(offensive_role));
	std::vector<player::ptr> defenders;
	std::vector<player::ptr> offenders;

	if (the_team.size() >= 3)
		defenders.push_back(the_team.get_player(2));

	if (the_team.size() >= 4)
		offenders.push_back(the_team.get_player(3));

	if (the_team.size() >= 5)
		defenders.push_back(the_team.get_player(4));
	
	// extra players become offenders
	for (size_t i = 5; i < the_team.size(); ++i)
		offenders.push_back(the_team.get_player(i));

	defensive_role->set_robots(defenders);
	offensive_role->set_robots(offenders);

	return the_team.get_player(1);
}

void basic_strategy::reset_all() {
	const friendly_team &the_team(the_world->friendly);

	roles.clear();

	if (the_team.size() == 0) return;

	// goalie only
	const std::vector<player::ptr> goalie_only(1, the_team.get_player(0));

	std::vector<player::ptr> all_players;

	// goalie is the first player
	for (size_t i = 0; i < the_team.size(); i++) {
		all_players.push_back(the_team.get_player(i));
	}

	// non goalie
	std::vector<player::ptr> all_but_goalie;

	// goalie is the first player
	for (size_t i = 1; i < the_team.size(); i++) {
		all_but_goalie.push_back(the_team.get_player(i));
	}

	// shooter for penalty kick
	std::vector<player::ptr> shooter_only;

	if (all_but_goalie.size() > 0) {
		shooter_only.push_back(all_but_goalie[0]);
	}

	std::vector<player::ptr> non_shooters;
	
	for (size_t i = 1; i < all_but_goalie.size(); ++i) {
		non_shooters.push_back(all_but_goalie[i]);
	}
	
	switch (the_world->playtype()) {
		case playtype::play:
		case playtype::stop:
		case playtype::execute_direct_free_kick_enemy:
		case playtype::execute_indirect_free_kick_enemy:
		case playtype::execute_kickoff_enemy:
			in_play_assignment();
			std::cout << the_team.size() << " robots set to play" << std::endl;
			break;

		case playtype::halt:
			break;

		case playtype::prepare_kickoff_friendly: 
		case playtype::execute_kickoff_friendly:
		case playtype::prepare_kickoff_enemy: 
			roles.push_back(role::ptr(new kickoff_friendly(the_world)));
			roles[0]->set_robots(all_but_goalie);
			std::cout << the_team.size() << " robots set to execute kickoff friendly" << std::endl;
			break;

		/*
		case playtype::prepare_kickoff_enemy:
			roles.push_back(role::ptr(new prepare_kickoff_enemy(the_world)));
			roles[0]->set_robots(all_but_goalie);
			std::cout << the_team.size() << " robots set to prepare kickoff enemy" << std::endl;
			break;
		*/
		// execute_kickoff_enemy isn't here; we use normal play assignment instead

		case playtype::prepare_penalty_friendly: 
			roles.push_back(role::ptr(new penalty_friendly(the_world)));
			roles[0]->set_robots(all_but_goalie);
			std::cout << the_team.size() << " robots set to prepare penalty friendly" << std::endl;
			break;

		case playtype::execute_penalty_friendly: 
			roles.push_back(role::ptr(new penalty_friendly(the_world)));
			roles[0]->set_robots(all_but_goalie);
			std::cout << the_team.size() << " robots set to execute penalty friendly" << std::endl;
			break;

		case playtype::execute_penalty_enemy:
			// "Ready" signal is meant to signal kicker may proceed,
			// so no difference on defending side.
			// May need to detect when the play type needs to be changed to play
		case playtype::prepare_penalty_enemy:
			roles.push_back(role::ptr(new penalty_enemy(the_world)));
			roles[0]->set_robots(all_but_goalie);
			roles.push_back(role::ptr(new penalty_goalie(the_world)));
			roles[1]->set_robots(goalie_only);
			std::cout << all_but_goalie.size() << " robots set to penalty enemy" << std::endl;
			std::cout << goalie_only.size() << " robots set to penalty goalie" << std::endl;
			break;

		case playtype::execute_direct_free_kick_friendly:
			{
				player::ptr one = minus_one_assignment();
				if (one) {
					std::vector<player::ptr> freekicker(1, one);
					//roles.push_back(role::ptr(new execute_direct_free_kick_friendly(the_world)));
					execute_direct_free_kick::ptr freekicker_role = execute_direct_free_kick::ptr(new execute_direct_free_kick(the_world));
					freekicker_role->set_robots(freekicker);
					//roles[0]->set_robots(all_but_goalie);
					roles.push_back(freekicker_role);
					std::cout << all_but_goalie.size() << " robots set to execute direct free kick friendly" << std::endl;
				}
			}
			break;

		case playtype::execute_indirect_free_kick_friendly:
			{
				player::ptr one = minus_one_assignment();
				if (one) {
					std::vector<player::ptr> freekicker(1, one);
					execute_indirect_free_kick::ptr freekicker_role = execute_indirect_free_kick::ptr(new execute_indirect_free_kick(the_world));
					freekicker_role->set_robots(freekicker);
					roles.push_back(freekicker_role);
					std::cout << all_but_goalie.size() << " robots set to execute indirect free kick" << std::endl;
				}
			}
			break;

		case playtype::pit_stop:
			roles.push_back(role::ptr(new pit_stop(the_world)));
			all_but_goalie.push_back(goalie_only[0]);
			roles[0]->set_robots(all_but_goalie);
			std::cout << all_but_goalie.size() << " robots set to pit stop" << std::endl;
			break;		

		case playtype::victory_dance:
			roles.push_back(role::ptr(new victory_dance));
			all_but_goalie.push_back(goalie_only[0]);
			roles[0]->set_robots(all_but_goalie);
			std::cout << all_but_goalie.size() << " robots set to victory dance" << std::endl;
			break;

		default:
			std::cerr << "Unhandled Playtype" << std::endl;
			break;
	}

	// Only assign goalie role for valid play type
	// In these cases the goalie role is the last role in the vector.
	switch (the_world->playtype()) {
		case playtype::halt:
		case playtype::stop:
		case playtype::play:
		//delete next line in case we don't use normal play assignment for execute_kickoff_enemy
		case playtype::execute_kickoff_enemy:
		case playtype::prepare_penalty_enemy:
		case playtype::execute_penalty_enemy:
		case playtype::pit_stop:
		case playtype::victory_dance:
		case playtype::execute_direct_free_kick_enemy:
		case playtype::execute_indirect_free_kick_enemy:
		case playtype::execute_direct_free_kick_friendly:
		case playtype::execute_indirect_free_kick_friendly:
			break;
		default:
			std::cerr << "Unhandled Playtype" << std::endl;
		case playtype::prepare_kickoff_friendly: 
		case playtype::execute_kickoff_friendly:
		case playtype::prepare_kickoff_enemy:
		//uncomment next line in case we don't use normal play assignment for execute_kickoff_enemy
		//case playtype::execute_kickoff_enemy:
		case playtype::prepare_penalty_friendly:
		case playtype::execute_penalty_friendly:
			roles.push_back(role::ptr((new goalie(the_world))));
			roles[roles.size()-1]->set_robots(goalie_only);
			break;
	}
}

strategy_factory &basic_strategy::get_factory() {
	return factory;
}

//
//This is from kenneth simple strategy which is now deleted
//use this just for ideas don't just enable this code
//or else!!!
//

/*

  ///////////////////////
  // The REST is NOT TRUE NOW.
  // It is assumed that the role vector already has one role containing the goalie.
  // It should always keep that role unchanged.
  //////////////////////
  void kenneth_simple_strategy::in_play_assignment(void)
  {
    const friendly_team &the_team(the_world->friendly);
	const team &the_other_team(the_world->enemy);
	const ball::ptr the_ball(the_world->ball());
	const field &the_field(the_world->field());
    if (the_team.size()<1)
    {
       roles.clear();
       return;
    }
    if (the_team.size()==1)
    {
       roles.clear();
       defensive::ptr defensive_role = defensive::ptr(new defensive(the_world));
       defensive_role->set_goalie(goalie_player);
       roles.push_back(role::ptr(defensive_role));
       return;
    }
    //keep for future
    //int our_score = the_team.score();
    //int their_score = the_other_team.score();		//get our team's robots' position and distance to the ball.
    std::vector<robot_details> our_details_front;
    std::vector<robot_details> our_details_back;		
    unsigned int our_team_size = the_team.size();
    double our_distance_to_ball[our_team_size];
    std::vector<player::ptr> offenders;
    std::vector<player::ptr> defenders;
    //		std::cout << our_team_size << std::endl;
    for (unsigned int i = 0; i < our_team_size; i++)
      {
	///////////////////
	// This skips the goalie
	///////////////////
	if (the_team.get_player(i)==goalie_player)
	  {
	    continue;
	  }
	robot_details temp_details;
	temp_details.dist_to_ball = (the_ball->position()-the_team.get_player(i)->position()).len();
	our_distance_to_ball[i] = temp_details.dist_to_ball;
	temp_details.index = i;
	temp_details.x = the_team.get_player(i)->position().x;
	temp_details.y = the_team.get_player(i)->position().y;
	if (temp_details.x < the_ball->position().x)
	  {	our_details_back.push_back(temp_details);		}	// between our goal and the ball
	else
	  {	our_details_front.push_back(temp_details);		}	// between their goal and the ball
      }
    std::sort(our_distance_to_ball, our_distance_to_ball + (our_team_size - 1 )); // team size need - 1 because the goalie is skipped.
    std::sort(our_details_front.begin(), our_details_front.end() , d_cmp);
    std::sort(our_details_back.begin(), our_details_back.end() , d_cmp);
    unsigned int their_team_size = the_other_team.size();
    double their_distance_to_ball[their_team_size];
    for (unsigned int i = 0; i< their_team_size; i++)
      {
	point difference_vector = the_ball->position()-the_other_team.get_robot(i)->position();
	their_distance_to_ball[i]= difference_vector.len();
      }
    std::sort(their_distance_to_ball, their_distance_to_ball + their_team_size);
    
    ///////////////////////////////////////////
    // effective_team_size is original team size - 1 (goalie)
    // The code below should handle conversion between signed and unsigned.
    //////////////////////////////////////////
    int our_effective_team_size = 0;
    our_effective_team_size = (our_team_size & 0x7fffffff);		 
    if (our_effective_team_size>0)
      { our_effective_team_size--;
      }
    int their_effective_team_size = 0;
    their_effective_team_size = (their_team_size & 0x7fffffff);
    if (their_effective_team_size>0)
      { their_effective_team_size--;
      }

    /////////////////////////////////////
    // Calculate the number of front & de players.
    // Offset the real diff by the default diff by the diff of team size.
    // 
    ////////////////////////////////////
    int prefer_off_to_def_diff = DEFAULT_OFF_TO_DEF_DIFF + our_effective_team_size - their_effective_team_size;
    double ball_pos_ratio = the_ball->position().x * 2 / the_field.length();  // the relative position of the ball.
//    std::cout << "ball_pos_ratio" <<  ball_pos_ratio << std::endl;
    if ( ball_pos_ratio < DEFENSE_ZONE)
      {
	prefer_off_to_def_diff --;
	//	prefer_off_to_def_diff = std::max( prefer_off_to_def_diff-1, 0);
	//	if ( prefer_off_to_def_diff <  )
	//	std::cout << "watch out!" << std::endl; 
      }else if ( ball_pos_ratio > OFFENSE_ZONE)
      {
	prefer_off_to_def_diff = our_effective_team_size;  //the entire team go as offender
//	std::cout << "all in!" << std::endl;
      }

    if (prefer_off_to_def_diff>our_effective_team_size)
      {    prefer_off_to_def_diff = our_effective_team_size;
      }
    if (prefer_off_to_def_diff< -1 * our_effective_team_size)
      {    prefer_off_to_def_diff = -1 * our_effective_team_size;
      }

////////////
// TODO Check this calculation
////////////
    int prefer_defender_number = std::min((our_effective_team_size - prefer_off_to_def_diff)/2, our_effective_team_size);
    int prefer_offender_number = our_effective_team_size - prefer_defender_number;

///////
/// TODO Always assign one robot more to the defender if goalie has the ball.
////////////////    
    int min_defender_number = 0;
    int min_offender_number = 0;
#warning has_ball
    if (goalie_player->sense_ball())
    {
       min_defender_number = 1;
    }
    if (our_effective_team_size - min_defender_number >= 1)
    {
       min_offender_number = 1;
    }

    /////////////////////////////////////
    //make sure the nearest robot is always an offender, 
    //then assigns the next (prefer_offender_number - 1) robots in the front side to the offender role,
    //if there is not enough in the front side, pick from back side.
    ////////////////////////////////////
      
    //check if the nearest robot is in the front side
    bool nearest_robot_is_in_front;
    if (our_details_front.size() == 0)
      {	nearest_robot_is_in_front = false;
      } else
      {	if (our_details_back.size() == 0)
	  {	nearest_robot_is_in_front = true;
	  }else
	  {	nearest_robot_is_in_front = our_details_front[0].dist_to_ball < our_details_back[0].dist_to_ball;
	  }
      }
    if (nearest_robot_is_in_front)
      {	//put our_details_front[0]->index to the offender side
	offenders.push_back(the_team.get_player(our_details_front[0].index));
      }
    else
      {	//put our_details_back[0]->index to the offender side
	offenders.push_back(the_team.get_player(our_details_back[0].index));
      }

    //////////////////////////////////////
    // Assign the player to each role
    //////////////////////////////////////
    int assigned_offender_number = 1;
    for (unsigned int i = nearest_robot_is_in_front; i < our_details_front.size(); i++)
      {
	if (assigned_offender_number < prefer_offender_number)	
	  {	//put our_details_front[i]->index to the offender side	
	    offenders.push_back(the_team.get_player(our_details_front[i].index));
	    assigned_offender_number ++ ;	
	  }
	else
	  {	//put our_details_front[i]->index to the defender side	
	    defenders.push_back(the_team.get_player(our_details_front[i].index));
	  }
      }
    for (unsigned int i = 1-nearest_robot_is_in_front; i < our_details_back.size(); i++)
      {
	if (assigned_offender_number < prefer_offender_number)	
	  {	//put our_details_back[i].index to the offender side	
	    offenders.push_back(the_team.get_player(our_details_back[i].index));
	    assigned_offender_number ++ ;	
	  }
	else
	  {	//put our_details_back[i].index to the defender side	
	    defenders.push_back(the_team.get_player(our_details_back[i].index));
	  }
      }	//end of (prefer_offender_number != 0 )
  
    ///////////////////////////////
    // just some print statement.
    //////////////////////////////
  if (turn_since_last_update % 100)
    {
  //    std::cout << "off" << offenders.size() << std::endl;
  //    for (unsigned int i = 0; i<offenders.size(); i++)
//	{
//	  std::cout << offenders[i]->position().x << " " << offenders[i]->position().y << std::endl;
//	}
  //    std::cout << "def" << defenders.size() << std::endl;
 //     for (unsigned int i = 0; i<defenders.size(); i++)
//	{
//	  std::cout << defenders[i]->position().x << " " << defenders[i]->position().y << std::endl;
//	}
  //    std::cout << "ball:" <<  the_ball->position().x << " " << the_ball->position().y << std::endl;
  //  }

  ///////////////////////////
  // Keep the goalie role, add it & the front & de roles to roles vector
  ///////////////////////////
//    role::ptr tempRole = roles[0];
    roles.clear();
//    roles.push_back(tempRole);
    if (offenders.size() > 0)
    {
        roles.push_back(role::ptr(new offensive(the_world)));
        roles[roles.size()-1]->set_robots(offenders);
    }
    defensive::ptr defensive_role = defensive::ptr(new defensive(the_world));
    defensive_role->set_goalie(goalie_player);
    roles.push_back(role::ptr(defensive_role));
    if (defenders.size() > 0)
    {
//        roles.push_back(role::ptr(new defensive(the_world)));
        roles[roles.size()-1]->set_robots(defenders);
    }
//	std::cout << "IT IS HERE !!!!!!!!!!!!!!!!!" << std::endl;
  //	for (int i = 0; (assigned_offender_number < prefer_offender_number) && (nearest_robot_is_in_front+i < our_details_front.size()); i++)
  //	for (int i = 0; assigned_offender_number < prefer_offender_number; i++)
  
  
  //use later
 // 	if ( our_distance_to_ball[0] / possession_confidence < their_distance_to_ball[0] )  
//	{
//	}
//	else
//	{
//	}
    
    //can we get the goals' position?
		
    
    // For all non-empty role, call update.
		
    //============================================back up
    
    
    //  robot_detaiunsigned ls our_details[team->size()];
    //  for (int i = 0; i< team->size(); i++)
    //  {
    //  point difference_vector = ball->position()-team->get_robot(i)->position();
//     = difference_vector.len();
//    our_details[i].dist_to_ball = our_distance_to_ball[i];
//    our_details[i].index = i;
//    our_details[i].x = get_robot(i)->position().x;
//    our_details[i].y = get_robot(i)->position().y;
//    }
//      std::sort(our_distance_to_ball);
//      std::sort(our_details, our_details + our_details.length, x_cmp);
//      int ball_relative_pos = -1;
//      for (int i = 0; i< team->size(); i++)
//      {
//      if (
//     }
  }

*/
