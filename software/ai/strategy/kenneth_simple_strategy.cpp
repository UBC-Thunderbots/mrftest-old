#include "ai/strategy.h"
#include "ai/role.h"
#include <algorithm>
#include <vector>
using namespace std;
//created by Kenneth Lui, last updated 26 Oct 2009.

namespace {

	struct robot_details{
		int index;	// index in the team
		double x;	// x coord
		double y;	// y coord
		double distToBall;	//distance between the ball and this robot
	};
	
	bool x_cmp ( robot_details* a, robot_details* b)		// first ele is the one having smallest x
	{
		return a->x < b->x;
	}

	bool d_cmp ( robot_details* a, robot_details* b)		// first ele is the one having smallest dist to ball
	{
		return a->distToBall < b->distToBall;
	}

	class simple_strategy : public virtual strategy {
		public:
			simple_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			virtual void update();
			virtual void set_playtype(playtype::playtype t);
			virtual strategy_factory &get_factory();
			virtual Gtk::Widget *get_ui_controls();
      virtual void handleRobotAdded(void);
      virtual void handleRobotRemoved(unsigned int index, robot::ptr r);

		private:
			playtype::playtype current_playtype;
			static const int WAIT_AT_LEAST_TURN = 5;		// We need this because we don't want to make frequent changes
			int turnSinceLastUpdate;
			double possessionConfidence;
			static const int DEFAULT_OFF_TO_DEF_DIFF = 1;	// i.e. one more offender than defender
			// Create variables here (e.g. to store the roles).
	};

	simple_strategy::simple_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
		// Initialize variables here (e.g. create the roles).
    the_team->signal_robot_added().connect(sigc::mem_fun(*this, &simple_strategy::handleRobotAdded));
    the_team->signal_robot_removed().connect(sigc::mem_fun(*this, &simple_strategy::handleRobotRemoved));

		turnSinceLastUpdate = 0;
		possessionConfidence = 1.0;
		// problems: how do we keep track of roles?
	}

	void simple_strategy::update() {
		// Use the variables "the_ball", "the_field", and "the_team" to allocate players to roles.
		

		turnSinceLastUpdate++;	// doesn't have effect yet.
		
		//keep for future
		//int our_score = the_team->score();
		//int their_score = the_team->other()->score();

		//get our team's robots' position and distance to the ball.
		vector<robot_details*> ourDetails_front;
		vector<robot_details*> ourDetails_back;
		unsigned int our_team_size = the_team->size();
		double ourDistanceToBall[our_team_size];
		for (unsigned int i = 0; i < our_team_size; i++)
		{
			robot_details* tempDetails = new robot_details();		// memory leak!!
			tempDetails->distToBall = (the_ball->position()-the_team->get_robot(i)->position()).len();
			ourDistanceToBall[i] = tempDetails->distToBall;
			tempDetails->index = i;
			tempDetails->x = the_team->get_robot(i)->position().x;
			tempDetails->y = the_team->get_robot(i)->position().y;
			if (tempDetails->x < the_ball->position().x)
			{	ourDetails_back.push_back(tempDetails);		}	// between our goal and the ball
			else
			{	ourDetails_front.push_back(tempDetails);		}	// between their goal and the ball
		}
		sort(ourDistanceToBall, ourDistanceToBall + our_team_size);
		sort(ourDetails_front.begin(), ourDetails_front.end() , d_cmp);
		sort(ourDetails_back.begin(), ourDetails_back.end() , d_cmp);

		unsigned int their_team_size = the_team->other()->size();
		double theirDistanceToBall[their_team_size];
		for (unsigned int i = 0; i< their_team_size; i++)
		{
			point diffVec = the_ball->position()-the_team->other()->get_robot(i)->position();
			theirDistanceToBall[i]= diffVec.len();
		}
		sort(theirDistanceToBall, theirDistanceToBall + their_team_size);

		// effective_team_size is original team size - 1 (goalie)
		int our_effective_team_size = 0;
		if (our_team_size & 1<<31)
		  { our_effective_team_size = (our_team_size & 0x7fffffff);
		  }
		if (our_effective_team_size>0)
		  { our_effective_team_size--;
		  }
		int their_effective_team_size = 0;
		if (their_team_size & 1<<31)
		  { their_effective_team_size = (their_team_size & 0x7fffffff);
		  }
		if (their_effective_team_size>0)
		  { their_effective_team_size--;
		  }
		int prefer_off_to_def_diff = DEFAULT_OFF_TO_DEF_DIFF + our_effective_team_size - their_effective_team_size;
		if (prefer_off_to_def_diff>our_effective_team_size)
		  {    prefer_off_to_def_diff = our_effective_team_size;
		  }
		if (prefer_off_to_def_diff< -1 * our_effective_team_size)
		  {    prefer_off_to_def_diff = -1 * our_effective_team_size;
		  }
		int prefer_defender_number = min((our_effective_team_size - prefer_off_to_def_diff)/2, our_effective_team_size);
		int prefer_offender_number = our_effective_team_size - prefer_defender_number;
		
		if (prefer_offender_number == 0 )
		{
			//assign all robots to defend
		}
		else
		{
			//make sure the nearest robot is always an offender, 
			//then assigns the next (prefer_offender_number - 1) robots in the front side to the offender role,
			//if there is not enough in the front side, pick from back side.
			
			//check if the nearest robot is in the front side
			bool nearestInFront;
			if (ourDetails_front.size() == 0)
			{	nearestInFront = false;
 			} else
			{	if (ourDetails_back.size() == 0)
				{	nearestInFront = true;
	 			}else
				{	nearestInFront = ourDetails_front[0]->distToBall < ourDetails_back[0]->distToBall;
				}
			}
			if (nearestInFront)
			{	//put ourDetails_front[0]->index to the offender side
			}
			else
			{	//put ourDetails_back[0]->index to the offender side
			}
			int assigned_offender_number = 1;
			for (unsigned int i = nearestInFront; i < ourDetails_front.size(); i++)
			{
				if (assigned_offender_number < prefer_offender_number)	
				{	//put ourDetails_front[i]->index to the offender side	
					assigned_offender_number ++ ;	
				}
				else
				{	//put ourDetails_front[i]->index to the defender side	
				}
			}
			for (unsigned int i = 1-nearestInFront; i < ourDetails_back.size(); i++)
			{
				if (assigned_offender_number < prefer_offender_number)	
				{	//put ourDetails_back[i].index to the offender side	
					assigned_offender_number ++ ;	
				}
				else
				{	//put ourDetails_back[i].index to the defender side	
				}
			}
		}	//end of (prefer_offender_number != 0 )

//			for (int i = 0; (assigned_offender_number < prefer_offender_number) && (nearestInFront+i < ourDetails_front.size()); i++)
//			for (int i = 0; assigned_offender_number < prefer_offender_number; i++)
		

		//use later
/*		if ( ourDistanceToBall[0] / possessionConfidence < theirDistanceToBall[0] )  
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
		robot_details ourDetails[team->size()];
		for (int i = 0; i< team->size(); i++)
		{
			point diffVec = ball->position()-team->get_robot(i)->position();
			= diffVec.len();
			ourDetails[i].distToBall = ourDistanceToBall[i];
			ourDetails[i].index = i;
			ourDetails[i].x = get_robot(i)->position().x;
			ourDetails[i].y = get_robot(i)->position().y;
		}
		sort(ourDistanceToBall);
		sort(ourDetails, ourDetails + ourDetails.length, x_cmp);
		int ball_relative_pos = -1;
		for (int i = 0; i< team->size(); i++)
		{
			if (
		}*/
	}

	void simple_strategy::set_playtype(playtype::playtype t) {
		current_playtype = t;
	}
	
	Gtk::Widget *simple_strategy::get_ui_controls() {
		return 0;
	}

  void simple_strategy::handleRobotAdded(void){
  }

  void simple_strategy::handleRobotRemoved(unsigned int index, robot::ptr r){
  }

	class simple_strategy_factory : public virtual strategy_factory {
		public:
			simple_strategy_factory();
			virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	simple_strategy_factory::simple_strategy_factory() : strategy_factory("Simple Strategy") {
	}

	strategy::ptr simple_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new simple_strategy(ball, field, team));
		return s;
	}

	simple_strategy_factory factory;

	strategy_factory &simple_strategy::get_factory() {
		return factory;
	}
}

