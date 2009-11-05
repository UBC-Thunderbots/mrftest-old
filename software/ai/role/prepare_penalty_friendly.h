#ifndef AI_ROLE_PREPARE_PENALTY_FRIENDLY_H
#define AI_ROLE_PREPARE_PENALTY_FRIENDLY_H

#include "ai/role.h"

//
// Gets the robots to go to their prepare_penalty_friendly positions.
//
class prepare_penalty_friendly : public role {
	public:
		//
		// A pointer to a prepare_penalty_friendly role.
		//
		typedef Glib::RefPtr<prepare_penalty_friendly> ptr;

		//
		// Constructs a new prepare_penalty_friendly role.
		//
		prepare_penalty_friendly(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

