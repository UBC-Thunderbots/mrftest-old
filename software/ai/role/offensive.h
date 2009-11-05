#ifndef AI_ROLE_OFFENSIVE_H
#define AI_ROLE_OFFENSIVE_H

#include "ai/role.h"

//
// Gets the robots to go to their offensive positions.
//
class offensive : public role {
	public:
		//
		// A pointer to a offensive role.
		//
		typedef Glib::RefPtr<offensive> ptr;

		//
		// Constructs a new offensive role.
		//
		offensive(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:
		
};

#endif

