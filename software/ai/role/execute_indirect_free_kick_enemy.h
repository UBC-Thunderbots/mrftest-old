#ifndef AI_ROLE_EXECUTE_INDIRECT_FREE_KICK_ENEMY_H
#define AI_ROLE_EXECUTE_INDIRECT_FREE_KICK_ENEMY_H

#include "ai/role.h"

//
// Gets the robots to go to their execute_indirect_free_kick_enemy positions.
//
class execute_indirect_free_kick_enemy : public role {
	public:
		//
		// A pointer to a execute_indirect_free_kick_enemy role.
		//
		typedef Glib::RefPtr<execute_indirect_free_kick_enemy> ptr;

		//
		// Constructs a new execute_indirect_free_kick_enemy role.
		//
		execute_indirect_free_kick_enemy(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:
		
};

#endif

