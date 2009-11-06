#ifndef AI_ROLE_DEFENSIVE_H
#define AI_ROLE_DEFENSIVE_H

#include "ai/role.h"

//
// Gets the robots to go to their defensive positions.
//
class defensive : public role {
	public:
		//
		// A pointer to a defensive role.
		//
		typedef Glib::RefPtr<defensive> ptr;

		//
		// Constructs a new defensive role.
		//
		defensive(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:
		
};

#endif

