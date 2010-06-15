#ifndef AI_ROLE_PREPARE_KICKOFF_FRIENDLY_H
#define AI_ROLE_PREPARE_KICKOFF_FRIENDLY_H

#include "ai/role/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "geom/point.h"


//
// Gets the robots to go to their prepare_kickoff_friendly positions.
//
class kickoff_friendly : public role {
	public:
		//
		// A pointer to a prepare_kickoff_friendly role.
		//
		typedef Glib::RefPtr<kickoff_friendly> ptr;

		//
		// Constructs a new prepare_kickoff_friendly role.
		//
		kickoff_friendly(world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();
		
		//
		//
		//don't need to do anything here
		void robots_changed(){
		}
		
	private:
	
		//
		//sees if team is in compliance of rules
		//
		bool team_compliance();
		
		point approach_legal_point(point cur_point,unsigned int robot_num);
		
		//
		//
		//
		world::ptr the_world;

		//
		// Is in rule violation
		//
		bool rule_violation(point cur_point);
		
		double circle_radius;
};

#endif

