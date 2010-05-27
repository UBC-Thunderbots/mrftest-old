#ifndef AI_ROLE_PREPARE_PENALTY_FRIENDLY_H
#define AI_ROLE_PREPARE_PENALTY_FRIENDLY_H

#include <vector>
#include "ai/role/role.h"
#include "ai/tactic/move.h"

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
		prepare_penalty_friendly(world::ptr world);

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

		/**
		 * The distance between the penalty mark and the mid point of the two goal posts as described in the rules.
		 */
		const static double PENALTY_MARK_LENGTH = 0.45;

		/**
		 * The distance between the baseline and the line behind which other robots may stand.
		 */
		const static double RESTRICTED_ZONE_LENGTH = 0.85;

	private:
		std::vector<move::ptr> the_tactics;

		/**
		 * Maximum number of robots that can be assigned to this role.
		 */
		const static unsigned int NUMBER_OF_READY_POSITIONS = 4;

		/**
		 * The positions that the robots should move to for this role.
		 */
		point ready_positions[NUMBER_OF_READY_POSITIONS];
};

#endif

