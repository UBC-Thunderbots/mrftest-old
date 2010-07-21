#ifndef AI_ROLE_PREPARE_KICKOFF_FRIENDLY_H
#define AI_ROLE_PREPARE_KICKOFF_FRIENDLY_H

#include "ai/role/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "geom/point.h"
#include "ai/world/player.h"

//
// Gets the robots to go to their prepare_kickoff_friendly positions.
//
class KickoffFriendly : public Role {
	public:
		//
		// A pointer to a KickoffFriendly Role.
		//
		typedef RefPtr<KickoffFriendly> ptr;

		//
		// Constructs a new KickoffFriendly Role.
		//
		KickoffFriendly(World::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();
		
		//
		//
		//don't need to do anything here
		void players_changed(){
		}
		
		//
		// Is in rule violation
		//
		bool rule_violation(Point cur_point);
		
	private:
	
		//
		//sees if team is in compliance of rules
		//
		bool team_compliance();
		
		Point approach_legal_point(Point cur_point,unsigned int robot_num);
		
		//
		//
		//
		World::ptr the_world;

		Point clip_circle(Point cur_point, double circle_radius, Point dst);

		
		double circle_radius;
};

#endif

