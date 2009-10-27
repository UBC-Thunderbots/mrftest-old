#ifndef AI_ROLE_EXECUTE_KICKOFF_FRIENDLY_H
#define AI_ROLE_EXECUTE_KICKOFF_FRIENDLY_H

#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"
#include <glibmm.h>
#include <sigc++/sigc++.h>
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
		void update();

	protected:
		
};

#endif

