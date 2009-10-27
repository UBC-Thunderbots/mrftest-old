#ifndef AI_ROLE_GOALIE_H
#define AI_ROLE_GOALIE_H

#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"
#include <glibmm.h>
#include <sigc++/sigc++.h>
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
		void update();

	protected:
		
};

#endif

