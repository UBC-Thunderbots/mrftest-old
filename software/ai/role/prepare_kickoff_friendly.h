#ifndef AI_ROLE_PREPARE_KICKOFF_FRIENDLY_H
#define AI_ROLE_PREPARE_KICKOFF_FRIENDLY_H

#include "ai/role/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "geom/point.h"


//
// Gets the robots to go to their prepare_kickoff_friendly positions.
//
class prepare_kickoff_friendly : public role {
	public:
		//
		// A pointer to a prepare_kickoff_friendly role.
		//
		typedef Glib::RefPtr<prepare_kickoff_friendly> ptr;

		//
		// Constructs a new prepare_kickoff_friendly role.
		//
		prepare_kickoff_friendly(world::ptr world);

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

                static const unsigned int NUMBER_OF_STARTING_POSITIONS = 5;

                point starting_positions[NUMBER_OF_STARTING_POSITIONS];
};

#endif

