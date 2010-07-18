#ifndef AI_ROLE_PREPARE_KICKOFF_ENEMY_H
#define AI_ROLE_PREPARE_KICKOFF_ENEMY_H

#include "ai/role/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "geom/point.h"

//
// Gets the robots to go to their prepare kickoff enemy positions.
//
class PrepareKickoffEnemy : public Role {
	public:
		//
		// A pointer to a PrepareKickoffEnemy Role.
		//
		typedef Glib::RefPtr<PrepareKickoffEnemy> ptr;

		//
		// Constructs a new PrepareKickoffEnemy Role.
		//
		PrepareKickoffEnemy(World::ptr world);

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

                static const unsigned int NUMBER_OF_STARTING_POSITIONS = 5;

                Point starting_positions[NUMBER_OF_STARTING_POSITIONS];

};

#endif

