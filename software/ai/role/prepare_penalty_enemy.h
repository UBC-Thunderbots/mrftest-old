#ifndef AI_ROLE_PREPARE_PENALTY_ENEMY_H
#define AI_ROLE_PREPARE_PENALTY_ENEMY_H

#include <vector>
#include "ai/role/role.h"
#include "ai/tactic/move.h"

//
// Gets the robots to go to their prepare penalty enemy positions.
//
class PreparePenaltyEnemy : public Role {
	public:
		//
		// A pointer to a PreparePenaltyEnemy Role.
		//
		typedef Glib::RefPtr<PreparePenaltyEnemy> ptr;

		//
		// Constructs a new PreparePenaltyEnemy Role.
		//
		PreparePenaltyEnemy(World::ptr world);

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

	private:
		/**
		 * The distance between the baseline and the line behind which the robots must stand.
		 */
		const static double RESTRICTED_ZONE_LENGTH = 0.85;

		/**
		* The tactics to be executed to move the robots.
		*/
		std::vector<Move::ptr> the_tactics;

		/**
		* Maximum number of positions that can be assigned for this Role.
		*/
		const static unsigned int NUM_POSITIONS = 4;

		/**
		* The designated standing positions for this Role.
		*/
		Point standing_positions[NUM_POSITIONS];
};

#endif

