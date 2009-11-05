#ifndef AI_ROLE_PIT_STOP_H
#define AI_ROLE_PIT_STOP_H

#include "ai/role.h"

//
// Gets the robots to go to their pit_stop positions.
//
class pit_stop : public role {
	public:
		//
		// A pointer to a pit_stop role.
		//
		typedef Glib::RefPtr<pit_stop> ptr;

		//
		// Constructs a new pit_stop role.
		//
		pit_stop(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

