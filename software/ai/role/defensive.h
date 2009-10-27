#ifndef AI_ROLE_DEFENSIVE_H
#define AI_ROLE_DEFENSIVE_H

#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"
#include <glibmm.h>
#include <sigc++/sigc++.h>
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
		void update();

	protected:
		
};

#endif

