#ifndef AI_ROLE_BYRONS_DEFENDER_H
#define AI_ROLE_BYRONS_DEFENDER_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

class byrons_defender : public role {
	public:
		//
		// A pointer to a byrons_defender role.
		//
		typedef Glib::RefPtr<byrons_defender> ptr;

		//
		// Constructs a new byrons_defender role.
		//
		byrons_defender(world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:
	
		const world::ptr the_world;

};

#endif

