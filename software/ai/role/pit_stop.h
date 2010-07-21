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
		typedef RefPtr<PitStop> Ptr;

		//
		// Constructs a new PitStop Role.
		//
		PitStop(World::Ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void players_changed();

	protected:
		const World::Ptr the_world;
		std::vector<Move::Ptr> the_tactics;
};

#endif

