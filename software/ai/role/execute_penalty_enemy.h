#ifndef AI_ROLE_EXECUTE_PENALTY_ENEMY_H
#define AI_ROLE_EXECUTE_PENALTY_ENEMY_H

#include "ai/role.h"

//
// Gets the robots to go to their execute_penalty_enemy positions.
//
class execute_penalty_enemy : public role {
	public:
		//
		// A pointer to a execute_penalty_enemy role.
		//
		typedef Glib::RefPtr<execute_penalty_enemy> ptr;

		//
		// Constructs a new execute_penalty_enemy role.
		//
		execute_penalty_enemy(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

