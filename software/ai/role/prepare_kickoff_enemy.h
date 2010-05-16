#ifndef AI_ROLE_PREPARE_KICKOFF_ENEMY_H
#define AI_ROLE_PREPARE_KICKOFF_ENEMY_H

#include "ai/role/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "geom/point.h"

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
		prepare_kickoff_enemy(world::ptr world);

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

