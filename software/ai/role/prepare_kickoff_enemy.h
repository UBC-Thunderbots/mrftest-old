#ifndef AI_ROLE_PREPARE_KICKOFF_ENEMY_H
#define AI_ROLE_PREPARE_KICKOFF_ENEMY_H

#include "ai/role.h"

//
// Gets the robots to go to their prepare_kickoff_enemy positions.
//
class prepare_kickoff_enemy : public role {
	public:
		//
		// A pointer to a prepare_kickoff_enemy role.
		//
		typedef Glib::RefPtr<prepare_kickoff_enemy> ptr;

		//
		// Constructs a new prepare_kickoff_enemy role.
		//
		prepare_kickoff_enemy(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:
		
};

#endif

