#ifndef AI_ROLE_PREPARE_PENALTY_ENEMY_H
#define AI_ROLE_PREPARE_PENALTY_ENEMY_H

#include "ai/role.h"

//
// Gets the robots to go to their prepare_penalty_enemy positions.
//
class prepare_penalty_enemy : public role {
	public:
		//
		// A pointer to a prepare_penalty_enemy role.
		//
		typedef Glib::RefPtr<prepare_penalty_enemy> ptr;

		//
		// Constructs a new prepare_penalty_enemy role.
		//
		prepare_penalty_enemy(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

