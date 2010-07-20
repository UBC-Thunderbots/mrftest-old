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
		// Constructs a new PitStop Role.
		//
		PitStop(RefPtr<World> world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void players_changed();

	protected:
		const RefPtr<World> the_world;
		std::vector<RefPtr<Move> > the_tactics;
};

#endif

