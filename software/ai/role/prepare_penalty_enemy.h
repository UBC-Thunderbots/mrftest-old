#ifndef AI_ROLE_PREPARE_PENALTY_ENEMY_H
#define AI_ROLE_PREPARE_PENALTY_ENEMY_H

#include <vector>
#include "ai/role/role.h"
#include "ai/tactic/move.h"

//
// Gets the robots to go to their prepare_penalty_enemy positions.
//
class prepare_penalty_enemy : public role {
	public:
		//
		// A pointer to a prepare_penalty_enemy role.
		//
		typedef Glib::RefPtr<prepare_penalty_enemy> ptr;

		//
		// Constructs a new prepare_penalty_enemy role.
		//
		prepare_penalty_enemy(world::ptr world);

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

	private:
		/**
		 * The distance between the baseline and the line behind which the robots must stand.
		 */
		const static double RESTRICTED_ZONE_LENGTH = 0.85;

		/**
		* The tactics to be executed to move the robots.
		*/
		std::vector<move::ptr> the_tactics;

		/**
		* Maximum number of positions that can be assigned for this role.
		*/
		const static unsigned int NUM_POSITIONS = 4;

		/**
		* The designated standing positions for this role.
		*/
		point standing_positions[NUM_POSITIONS];
};

#endif

