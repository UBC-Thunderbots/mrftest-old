#include "ai/strategy.h"
#include "ai/role.h"
#include "ai/tactic.h"
#include "ai/tactic/chase.h"
#include <algorithm>
#include <vector>
//created by Kenneth Lui, last updated 2 Nov 2009.
//This strategy was created to test the simulator.

namespace {

	struct robot_details{
		int index;	// index in the team
		double x;	// x coord
		double y;	// y coord
		double dist_to_ball;	//distance between the ball and this robot
	};
	
	bool x_cmp ( robot_details* a, robot_details* b)		// first ele is the one having smallest x
	{
		return a->x < b->x;
	}

	bool d_cmp ( robot_details* a, robot_details* b)		// first ele is the one having smallest dist to ball
	{
		return a->dist_to_ball < b->dist_to_ball;
	}

	class simu_test_strategy : public virtual strategy {
		public:
			simu_test_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			virtual void update();
			virtual void set_playtype(playtype::playtype t);
			virtual strategy_factory &get_factory();
			virtual Gtk::Widget *get_ui_controls();
			virtual void robot_added(void);
			virtual void robot_removed(unsigned int index, robot::ptr r);

		private:
			playtype::playtype current_playtype;
			static const int WAIT_AT_LEAST_TURN = 5;		// We need this because we don't want to make frequent changes
			int turn_since_last_update;
			double possession_confidence;
			static const int DEFAULT_OFF_TO_DEF_DIFF = 1;	// i.e. one more offender than defender
			std::vector<tactic::ptr> tactics;
			// Create variables here (e.g. to store the roles).
	};

	simu_test_strategy::simu_test_strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : strategy(ball, field, team) {
		// Initialize variables here (e.g. create the roles).
		turn_since_last_update = 0;
		possession_confidence = 1.0;
		for (unsigned int i = 0; i < the_team->size(); i++)
		{
			tactics.push_back(tactic::ptr(new chase(the_ball, the_field, the_team, the_team->get_player(i))));
		}
		return;
		// problems: how do we keep track of roles?
	}

	void simu_test_strategy::update() {
		// Use the variables "the_ball", "the_field", and "the_team" to allocate players to roles.
		switch (current_playtype)
		{
			case playtype::play:	for (unsigned int i = 0; i < tactics.size();i++)
						{
							tactics[i]->update();
						}
						turn_since_last_update++;	// doesn't have effect yet.
						break;
			default	:		break;
		}
		//keep for future
		//int our_score = the_team->score();
		//int their_score = the_team->other()->score();

		//get our team's robots' position and distance to the ball.
		std::vector<robot_details*> our_details_front;
		std::vector<robot_details*> our_details_back;
		unsigned int our_team_size = the_team->size();
		double our_distance_to_ball[our_team_size];
		for (unsigned int i = 0; i < our_team_size; i++)
		{
			robot_details* temp_details = new robot_details();		// memory leak!!
			temp_details->dist_to_ball = (the_ball->position()-the_team->get_robot(i)->position()).len();
			our_distance_to_ball[i] = temp_details->dist_to_ball;
			temp_details->index = i;
			temp_details->x = the_team->get_robot(i)->position().x;
			temp_details->y = the_team->get_robot(i)->position().y;
			if (temp_details->x < the_ball->position().x)
			{	our_details_back.push_back(temp_details);		}	// between our goal and the ball
			else
			{	our_details_front.push_back(temp_details);		}	// between their goal and the ball
		}
		std::sort(our_distance_to_ball, our_distance_to_ball + our_team_size);
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
		int prefer_defender_number = std::min((our_effective_team_size - prefer_off_to_def_diff)/2, our_effective_team_size);
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
			bool nearest_robot_is_in_front;
			if (our_details_front.size() == 0)
			{	nearest_robot_is_in_front = false;
 			} else
			{	if (our_details_back.size() == 0)
				{	nearest_robot_is_in_front = true;
	 			}else
				{	nearest_robot_is_in_front = our_details_front[0]->dist_to_ball < our_details_back[0]->dist_to_ball;
				}
			}
			if (nearest_robot_is_in_front)
			{	//put our_details_front[0]->index to the offender side
			}
			else
			{	//put our_details_back[0]->index to the offender side
			}
			int assigned_offender_number = 1;
			for (unsigned int i = nearest_robot_is_in_front; i < our_details_front.size(); i++)
			{
				if (assigned_offender_number < prefer_offender_number)	
				{	//put our_details_front[i]->index to the offender side	
					assigned_offender_number ++ ;	
				}
				else
				{	//put our_details_front[i]->index to the defender side	
				}
			}
			for (unsigned int i = 1-nearest_robot_is_in_front; i < our_details_back.size(); i++)
			{
				if (assigned_offender_number < prefer_offender_number)	
				{	//put our_details_back[i].index to the offender side	
					assigned_offender_number ++ ;	
				}
				else
				{	//put our_details_back[i].index to the defender side	
				}
			}
		}	//end of (prefer_offender_number != 0 )

//			for (int i = 0; (assigned_offender_number < prefer_offender_number) && (nearest_robot_is_in_front+i < our_details_front.size()); i++)
//			for (int i = 0; assigned_offender_number < prefer_offender_number; i++)
		

		//use later
/*		if ( our_distance_to_ball[0] / possession_confidence < their_distance_to_ball[0] )  
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
		robot_details our_details[team->size()];
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

	void simu_test_strategy::set_playtype(playtype::playtype t) {
		current_playtype = t;
	}
	
	Gtk::Widget *simu_test_strategy::get_ui_controls() {
		return 0;
	}

	void simu_test_strategy::robot_added(void) {
	}

	void simu_test_strategy::robot_removed(unsigned int index, robot::ptr r) {
	}

	class simu_test_strategy_factory : public virtual strategy_factory {
		public:
			simu_test_strategy_factory();
			virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	simu_test_strategy_factory::simu_test_strategy_factory() : strategy_factory("Simulator Test Strategy") {
	}

	strategy::ptr simu_test_strategy_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new simu_test_strategy(ball, field, team));
		return s;
	}

	simu_test_strategy_factory factory;

	strategy_factory &simu_test_strategy::get_factory() {
		return factory;
	}
}

