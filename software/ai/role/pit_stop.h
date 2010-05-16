#ifndef AI_ROLE_PIT_STOP_H
#define AI_ROLE_PIT_STOP_H

#include "ai/role/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "geom/point.h"

//
// Gets the robots to go to their pit_stop positions.
//
class pit_stop : public role {
	public:
		//
		// A pointer to a pit_stop role.
		//
		typedef Glib::RefPtr<pit_stop> ptr;

		//
		// Constructs a new pit_stop role.
		//
		pit_stop(world::ptr world);

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
                std::vector<move::ptr> the_tactics;	
};

#endif

