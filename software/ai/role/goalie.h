#ifndef AI_ROLE_GOALIE_H
#define AI_ROLE_GOALIE_H

#include "ai/role.h"

//
// Gets the robots to go to their goalie positions.
//
class goalie : public role {
	public:
		//
		// A pointer to a goalie role.
		//
		typedef Glib::RefPtr<goalie> ptr;

		//
		// Constructs a new goalie role.
		//
		goalie(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:
		
};

#endif

