#ifndef AI_ROLE_PIT_STOP_H
#define AI_ROLE_PIT_STOP_H

#include "ai/role/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "geom/point.h"

//
// Gets the robots to go to their pit stop positions.
//
class PitStop : public Role {
	public:
		//
		// A pointer to a PitStop Role.
		//
		typedef Glib::RefPtr<PitStop> ptr;

		//
		// Constructs a new PitStop Role.
		//
		PitStop(World::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:
		const World::ptr the_world;
                std::vector<Move::ptr> the_tactics;	
};

#endif

