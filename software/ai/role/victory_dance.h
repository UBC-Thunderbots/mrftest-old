#ifndef AI_ROLE_VICTORY_DANCE_H
#define AI_ROLE_VICTORY_DANCE_H

#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"
#include <glibmm.h>
#include <sigc++/sigc++.h>
#include "ai/role.h"

//
// Gets the robots to go to their victory_dance positions.
//
class victory_dance : public role {
	public:
		//
		// A pointer to a victory_dance role.
		//
		typedef Glib::RefPtr<victory_dance> ptr;

		//
		// Constructs a new victory_dance role.
		//
		victory_dance(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

