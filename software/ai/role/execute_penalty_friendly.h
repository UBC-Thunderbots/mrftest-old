#ifndef AI_ROLE_EXECUTE_PENALTY_FRIENDLY_H
#define AI_ROLE_EXECUTE_PENALTY_FRIENDLY_H

#include "ai/role.h"

//
// Gets the robots to go to their execute_penalty_friendly positions.
//
class execute_penalty_friendly : public role {
	public:
		//
		// A pointer to a execute_penalty_friendly role.
		//
		typedef Glib::RefPtr<execute_penalty_friendly> ptr;

		//
		// Constructs a new execute_penalty_friendly role.
		//
		execute_penalty_friendly(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

