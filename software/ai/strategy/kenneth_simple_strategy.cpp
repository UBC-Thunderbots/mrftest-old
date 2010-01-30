#include "ai/strategy.h"
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
#include "ai/role/halt.h"
#include "ai/role/stop.h"


#include <iostream>
//created by Kenneth Lui, last updated 29 Jan 2010.

namespace {
  struct robot_details{
    int index;	// index in the team
    double x;	// x coord
    double y;	// y coord
    double dist_to_ball;	//distance between the ball and this robot
  };
  
  bool x_cmp ( const robot_details &a, const robot_details &b)		// first ele is the one having smallest x
  {
    return a.x < b.x;
  }
  
  bool d_cmp ( const robot_details &a, const robot_details &b)		// first ele is the one having smallest dist to ball
  {
    return a.dist_to_ball < b.dist_to_ball;
  }
  
  class kenneth_simple_strategy : public strategy {
  public:
    kenneth_simple_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
    void tick();
    void set_playtype(playtype::playtype t);
    strategy_factory &get_factory();
    Gtk::Widget *get_ui_controls();
    void robot_added(void);
    void robot_removed(unsigned int index, player::ptr r);
    
  private:
    //private functions
    void reset_all(void);
    void in_play_assignment(void);
//    void exclude_goalie(std::vector<player::ptr>& players_vector);

    //private variables
    static const int WAIT_AT_LEAST_TURN = 5;		// We need this because we don't want to make frequent changes
    int turn_since_last_update;
    double possession_confidence;
    static const int DEFAULT_OFF_TO_DEF_DIFF = 1;	// i.e. one more offender than defender

    // 1 = all field, 0 = mid-field, -1 = none 
    // larger number -> larger defense zone
    static const double DEFENSE_ZONE = 0.15; 
    // 1 = none, 0 = mid-field, -1 = all-field
    // larger number -> smaller offense zone
    // this number should be >= DEFENSE_ZONE
    static const double OFFENSE_ZONE = 0.70; 
//    std::vector<tactic::ptr> tactics;
    std::vector<role::ptr> roles;
    player::ptr goalie_player;
  };
  
  kenneth_simple_strategy::kenneth_simple_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
    // Initialize variables here (e.g. create the roles).
    turn_since_last_update = 0;
    possession_confidence = 1.0;
    goalie_player = player::ptr(NULL);         // not sure if this is good
    //reset_all();
    return;
    // problems: how do we keep track of roles?
  }
  
  void kenneth_simple_strategy::tick() {
     // Use the variables "the_ball", "the_field", and "the_team" to allocate players to roles.
    turn_since_last_update++;
    if (turn_since_last_update % 40 == 0)
      {  std::cout << "tick" << turn_since_last_update << std::endl;
      }
    switch (the_team->current_playtype())
    {
//      case playtype::halt: break;
//      case playtype::stop: break;
      case playtype::play: 
        in_play_assignment();
        for (unsigned int i = 0; i < roles.size(); i++)
        {
            roles[i]->tick();
        }
	break;
	/*	case playtype::prepare_kickoff_friendly: break;
		case playtype::execute_kickoff_friendly: break;
		case playtype::prepare_kickoff_enemy: break;
		case playtype::execute_kickoff_enemy: break;
		case playtype::prepare_penalty_friendly: break;
		case playtype::execute_penalty_friendly: break;
		case playtype::prepare_penalty_enemy: break;
		case playtype::execute_penalty_enemy: break;
		case playtype::execute_direct_free_kick_friendly: break;
		case playtype::execute_indirect_free_kick_friendly: break;
		case playtype::execute_direct_free_kick_enemy: break;
		case playtype::execute_indirect_free_kick_enemy: break;
	        case playtype::pit_stop: for (unsigned int i = 0; i < roles.size();i++)
		{
		roles[i]->tick();
		}
		break;				      
		case playtype::victory_dance: break;*/
      default	:	 for (unsigned int i = 0; i < roles.size();i++)
	  {
	    roles[i]->tick();
            //std::cout << "role #" << i << " ticked" << std::endl;
	    //					    std::cout << "ticking" << std::endl;
	  }
	break;
      }
    return;
  }

  void kenneth_simple_strategy::set_playtype(playtype::playtype) { 
    reset_all();
  }
  
  Gtk::Widget *kenneth_simple_strategy::get_ui_controls() {
    return 0;
	}
 

  ///////////////////////
  // The REST is NOT TRUE NOW.
  // It is assumed that the role vector already has one role containing the goalie.
  // It should always keep that role unchanged.
  //////////////////////
  void kenneth_simple_strategy::in_play_assignment(void)
  {
    if (the_team->size()==1)
    {
       roles.clear();
       defensive::ptr defensive_role = defensive::ptr(new defensive(the_ball, the_field, the_team));
       defensive_role->set_goalie(goalie_player);
       roles.push_back(role::ptr(defensive_role));
       return;
    }
    //keep for future
    //int our_score = the_team->score();
    //int their_score = the_team->other()->score();		//get our team's robots' position and distance to the ball.
    std::vector<robot_details> our_details_front;
    std::vector<robot_details> our_details_back;		
    unsigned int our_team_size = the_team->size();
    double our_distance_to_ball[our_team_size];
    std::vector<player::ptr> offenders;
    std::vector<player::ptr> defenders;
    //		std::cout << our_team_size << std::endl;
    for (unsigned int i = 0; i < our_team_size; i++)
      {
	///////////////////
	// This skips the goalie
	///////////////////
	if (the_team->get_player(i)==goalie_player)
	  {
	    continue;
	  }
	robot_details temp_details;
	temp_details.dist_to_ball = (the_ball->position()-the_team->get_player(i)->position()).len();
	our_distance_to_ball[i] = temp_details.dist_to_ball;
	temp_details.index = i;
	temp_details.x = the_team->get_player(i)->position().x;
	temp_details.y = the_team->get_player(i)->position().y;
	if (temp_details.x < the_ball->position().x)
	  {	our_details_back.push_back(temp_details);		}	// between our goal and the ball
	else
	  {	our_details_front.push_back(temp_details);		}	// between their goal and the ball
      }
    std::sort(our_distance_to_ball, our_distance_to_ball + (our_team_size - 1 )); // team size need - 1 because the goalie is skipped.
    std::sort(our_details_front.begin(), our_details_front.end() , d_cmp);
    std::sort(our_details_back.begin(), our_details_back.end() , d_cmp);
    unsigned int their_team_size = the_team->other()->size();
    double their_distance_to_ball[their_team_size];
    for (unsigned int i = 0; i< their_team_size; i++)
      {
	point difference_vector = the_ball->position()-the_team->other()->get_robot(i)->position();
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
    double ball_pos_ratio = the_ball->position().x * 2 / the_field->length();  // the relative position of the ball.
//    std::cout << "ball_pos_ratio" <<  ball_pos_ratio << std::endl;
    if ( ball_pos_ratio < DEFENSE_ZONE)
      {
	prefer_off_to_def_diff --;
/*	prefer_off_to_def_diff = std::max( prefer_off_to_def_diff-1, 0);
	if ( prefer_off_to_def_diff <  )
	std::cout << "watch out!" << std::endl; */
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
    if (goalie_player->has_ball())
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
	offenders.push_back(the_team->get_player(our_details_front[0].index));
      }
    else
      {	//put our_details_back[0]->index to the offender side
	offenders.push_back(the_team->get_player(our_details_back[0].index));
      }

    //////////////////////////////////////
    // Assign the player to each role
    //////////////////////////////////////
    int assigned_offender_number = 1;
    for (unsigned int i = nearest_robot_is_in_front; i < our_details_front.size(); i++)
      {
	if (assigned_offender_number < prefer_offender_number)	
	  {	//put our_details_front[i]->index to the offender side	
	    offenders.push_back(the_team->get_player(our_details_front[i].index));
	    assigned_offender_number ++ ;	
	  }
	else
	  {	//put our_details_front[i]->index to the defender side	
	    defenders.push_back(the_team->get_player(our_details_front[i].index));
	  }
      }
    for (unsigned int i = 1-nearest_robot_is_in_front; i < our_details_back.size(); i++)
      {
	if (assigned_offender_number < prefer_offender_number)	
	  {	//put our_details_back[i].index to the offender side	
	    offenders.push_back(the_team->get_player(our_details_back[i].index));
	    assigned_offender_number ++ ;	
	  }
	else
	  {	//put our_details_back[i].index to the defender side	
	    defenders.push_back(the_team->get_player(our_details_back[i].index));
	  }
      }	//end of (prefer_offender_number != 0 )
  
    ///////////////////////////////
    // just some print statement.
    //////////////////////////////
/*  if (turn_since_last_update % 100)
    {
      std::cout << "off" << offenders.size() << std::endl;
      for (unsigned int i = 0; i<offenders.size(); i++)
	{
	  std::cout << offenders[i]->position().x << " " << offenders[i]->position().y << std::endl;
	}
      std::cout << "def" << defenders.size() << std::endl;
      for (unsigned int i = 0; i<defenders.size(); i++)
	{
	  std::cout << defenders[i]->position().x << " " << defenders[i]->position().y << std::endl;
	}
      std::cout << "ball:" <<  the_ball->position().x << " " << the_ball->position().y << std::endl;
    }
*/
  ///////////////////////////
  // Keep the goalie role, add it & the front & de roles to roles vector
  ///////////////////////////
//    role::ptr tempRole = roles[0];
    roles.clear();
//    roles.push_back(tempRole);
    if (offenders.size() > 0)
    {
        roles.push_back(role::ptr(new offensive(the_ball, the_field, the_team)));
        roles[roles.size()-1]->set_robots(offenders);
    }
    defensive::ptr defensive_role = defensive::ptr(new defensive(the_ball, the_field, the_team));
    defensive_role->set_goalie(goalie_player);
    roles.push_back(role::ptr(defensive_role));
    if (defenders.size() > 0)
    {
//        roles.push_back(role::ptr(new defensive(the_ball, the_field, the_team)));
        roles[roles.size()-1]->set_robots(defenders);
    }
//	std::cout << "IT IS HERE !!!!!!!!!!!!!!!!!" << std::endl;
  //	for (int i = 0; (assigned_offender_number < prefer_offender_number) && (nearest_robot_is_in_front+i < our_details_front.size()); i++)
  //	for (int i = 0; assigned_offender_number < prefer_offender_number; i++)
  
  
  //use later
  /*	if ( our_distance_to_ball[0] / possession_confidence < their_distance_to_ball[0] )  
	{
	}
	else
	{
	}
    */
    //can we get the goals' position?
		
    
    // For all non-empty role, call update.
		
    //============================================back up
    
    /*
      robot_detaiunsigned ls our_details[team->size()];
      for (int i = 0; i< team->size(); i++)
      {
      point difference_vector = ball->position()-team->get_robot(i)->position();
      = difference_vector.len();
      our_details[i].dist_to_ball = our_distance_to_ball[i];
      our_details[i].index = i;
      our_details[i].x = get_robot(i)->position().x;
      our_details[i].y = get_robot(i)->position().y;
      }
      std::sort(our_distance_to_ball);
      std::sort(our_details, our_details + our_details.length, x_cmp);
      int ball_relative_pos = -1;
      for (int i = 0; i< team->size(); i++)
      {
      if (
      }*/
  }

/*
  void kenneth_simple_strategy::exclude_goalie(std::vector<player::ptr>& players_vector)
  {
    //    std::cout << "exclude_goalie called" << std::endl;
    std::vector<player::ptr>::iterator itr = players_vector.begin();
    int i = 0;
    while (itr !=  players_vector.end())
      {
	if (*itr==goalie_player)
	  {
	    players_vector.erase(itr);    
	    //	    std::cout << "exclude_goalie!" << std::endl;
	    break;
	  }
	i++;
	itr++;
      }
    return;
  }
*/

  ////////////////////////////////////////
  /////Reset all is always called after a robot has been added/removed
  /////////////////////////////////////////
  void kenneth_simple_strategy::reset_all(void)
  {
    if (the_team->size()==0)
    {
        return;
    }else if (goalie_player == player::ptr(NULL))
    {
        //This case is used to handle the stupid remove calls when the simulator starts.
        return;
    }

    /////////////////////////////
    // Only contain everyone but the goalie
    /////////////////////////////
    std::vector<player::ptr> all_players;

    //////////////////////////
    // Only contain the goalie.
    ///////////////////////////
    std::vector<player::ptr> goalie_only;

    //////////////////////////
    // Fill the vectors
    //////////////////////////
    for (unsigned int i = 0; i < the_team->size(); i++)
      {
	if (goalie_player == the_team->get_player(i))
	  {
	    goalie_only.push_back(the_team->get_player(i));    
	  }else
	  {
	    all_players.push_back( the_team->get_player(i) );
	  }
      }

    ////////////////////
    // All roles are cleared
    ////////////////////
    roles.clear();

    goalie::ptr goalie_role;

    ////////////////////
    // Switch play type
    ///////////////////
    switch (the_team->current_playtype())
      {
      case playtype::halt:
        roles.push_back(role::ptr(new halt(the_ball, the_field, the_team)));
        all_players.push_back(goalie_only[0]);
        roles[0]->set_robots(all_players);
        std::cout << all_players.size() << " robots set to halt" << std::endl;
        break;

      case playtype::stop:
        roles.push_back(role::ptr(new stop(the_ball, the_field, the_team)));
        all_players.push_back(goalie_only[0]);
        roles[0]->set_robots(all_players);
        std::cout << all_players.size() << " robots set to stop" << std::endl;
        break;

      case playtype::play:
//            goalie_role = goalie::ptr(new goalie(the_ball, the_field, the_team));
//            goalie_role->start_play();
//            roles.push_back(role::ptr((goalie_role)));
//            roles[0]->set_robots(goalie_only);
            in_play_assignment();
            break;

      case playtype::prepare_kickoff_friendly: 
	roles.push_back(role::ptr(new prepare_kickoff_friendly(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to prepare kickoff friendly" << std::endl;
	break;
	
      case playtype::execute_kickoff_friendly: 
	roles.push_back(role::ptr(new execute_kickoff_friendly(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to execute kickoff friendly" << std::endl;
	break;

      case playtype::prepare_kickoff_enemy:
	roles.push_back(role::ptr(new prepare_kickoff_enemy(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to prepare kickoff enemy" << std::endl;
	break;
	
      case playtype::execute_kickoff_enemy:
	roles.push_back(role::ptr(new execute_kickoff_enemy(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to execute kickoff enemy" << std::endl;
	break;
	
      case playtype::prepare_penalty_friendly: 
	roles.push_back(role::ptr(new prepare_penalty_friendly(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to prepare penalty friendly" << std::endl;
	break;
	
      case playtype::execute_penalty_friendly: 
	roles.push_back(role::ptr(new execute_penalty_friendly(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to execute penalty friendly" << std::endl;
	break;
	
      case playtype::prepare_penalty_enemy:
	roles.push_back(role::ptr(new prepare_penalty_enemy(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to prepare penalty enemy" << std::endl;
	break;
	
      case playtype::execute_penalty_enemy:
	roles.push_back(role::ptr(new execute_penalty_enemy(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to execute penalty enemy" << std::endl;
	break;
	
      case playtype::execute_direct_free_kick_friendly:
	roles.push_back(role::ptr(new execute_direct_free_kick_friendly(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to execute direct free kick friendly" << std::endl;
	break;
	
      case playtype::execute_indirect_free_kick_friendly:
	roles.push_back(role::ptr(new execute_indirect_free_kick_friendly(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to execute indirect free kick friendly" << std::endl;
	break;
	
      case playtype::execute_direct_free_kick_enemy:
	roles.push_back(role::ptr(new execute_direct_free_kick_enemy(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to execute direct free kick enemy" << std::endl;
	break;
	
      case playtype::execute_indirect_free_kick_enemy:
	roles.push_back(role::ptr(new execute_indirect_free_kick_enemy(the_ball, the_field, the_team)));
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to execute indirect free kick friendly" << std::endl;
	break;
	
	////////
	// We need the goalie to go to pit stop too
	//////////
      case playtype::pit_stop:
	roles.push_back(role::ptr(new pit_stop(the_ball, the_field, the_team)));
	all_players.push_back(goalie_only[0]);
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to pit stop" << std::endl;
	break;		

	////////
	// We need the goalie to dance
	//////////	
      case playtype::victory_dance:
	roles.push_back(role::ptr(new victory_dance(the_ball, the_field, the_team)));
	all_players.push_back(goalie_only[0]);
	roles[0]->set_robots(all_players);
	std::cout << all_players.size() << " robots set to victory dance" << std::endl;
	break;
      default	:		break;
      }

    ///////////////////////
    // Only assign goalie role for valid play type
    // In these cases the goalie role is the last role in the vector.
    ///////////////////////
    switch (the_team->current_playtype())
    {
        case playtype::halt: break;
        case playtype::stop: break;
	case playtype::play: break;
        case playtype::pit_stop: break;
        case playtype::victory_dance: break;
        default:
            roles.push_back(role::ptr((new goalie(the_ball, the_field, the_team))));
            roles[roles.size()-1]->set_robots(goalie_only);
            std::cout << goalie_only.size() << " robots set to goalie" << std::endl;
            break;
    }    
  }

  void kenneth_simple_strategy::robot_added(void) {
    std::cout << "<<<<<<<<<ROBOT ADDED>>>>" << std::endl;	 
    if ((the_team->size()==1) || (goalie_player==player::ptr(NULL)) )
      {
	goalie_player = the_team->get_player(0);
	std::cout << "new goalie robot assigned" << std::endl;	      
      }
    reset_all(); 
  }
  
  void kenneth_simple_strategy::robot_removed(unsigned int, player::ptr r) {	  
    std::cout << "<<<<<<<<<ROBOT Removed>>>>" << std::endl;
    if (r==goalie_player)
      {
	std::cout << "goalie_player removed" << std::endl;
	if (the_team->size()>=1)
	  {
	    goalie_player = the_team->get_player(0);
	    //std::cout << goalie_player << std::endl;
	    std::cout << "new goalie player assigned" << std::endl;	      
	  }else
	  {
	    goalie_player = player::ptr(NULL);
	  }
      }
    reset_all();
  }
  
  class kenneth_simple_strategy_factory : public strategy_factory {
  public:
    kenneth_simple_strategy_factory();
    strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
  };

  kenneth_simple_strategy_factory::kenneth_simple_strategy_factory() : strategy_factory("Kenneth Simple Strategy") {
  }

  strategy::ptr kenneth_simple_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
    strategy::ptr s(new kenneth_simple_strategy(ball, field, team));
    return s;
  }
  
  kenneth_simple_strategy_factory factory;
  
  strategy_factory &kenneth_simple_strategy::get_factory() {
    return factory;
  }
}

