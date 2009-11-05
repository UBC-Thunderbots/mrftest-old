#ifndef AI_ROLE_EXECUTE_INDIRECT_FREE_KICK_FRIENDLY_H
#define AI_ROLE_EXECUTE_INDIRECT_FREE_KICK_FRIENDLY_H

#include "ai/role.h"

//
// Gets the robots to go to their execute_indirect_free_kick_friendly positions.
//
class execute_indirect_free_kick_friendly : public role {
	public:
		//
		// A pointer to a execute_indirect_free_kick_friendly role.
		//
		typedef Glib::RefPtr<execute_indirect_free_kick_friendly> ptr;

		//
		// Constructs a new execute_indirect_free_kick_friendly role.
		//
		execute_indirect_free_kick_friendly(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

