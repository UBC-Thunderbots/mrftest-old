#ifndef AI_ROLE_EXECUTE_DIRECT_FREE_KICK_ENEMY_H
#define AI_ROLE_EXECUTE_DIRECT_FREE_KICK_ENEMY_H

#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"
#include <glibmm.h>
#include <sigc++/sigc++.h>
#include "ai/role.h"

//
// Gets the robots to go to their execute_direct_free_kick_enemy positions.
//
class execute_direct_free_kick_enemy : public role {
	public:
		//
		// A pointer to a execute_direct_free_kick_enemy role.
		//
		typedef Glib::RefPtr<execute_direct_free_kick_enemy> ptr;

		//
		// Constructs a new execute_direct_free_kick_enemy role.
		//
		execute_direct_free_kick_enemy(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

