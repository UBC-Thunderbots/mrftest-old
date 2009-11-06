#ifndef AI_ROLE_EXECUTE_KICKOFF_FRIENDLY_H
#define AI_ROLE_EXECUTE_KICKOFF_FRIENDLY_H

#include "ai/role.h"

//
// Gets the robots to go to their execute_kickoff_friendly positions.
//
class execute_kickoff_friendly : public role {
	public:
		//
		// A pointer to a execute_kickoff_friendly role.
		//
		typedef Glib::RefPtr<execute_kickoff_friendly> ptr;

		//
		// Constructs a new execute_kickoff_friendly role.
		//
		execute_kickoff_friendly(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:
		
};

#endif

