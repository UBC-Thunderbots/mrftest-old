#ifndef AI_ROLE_PREPARE_PENALTY_ENEMY_H
#define AI_ROLE_PREPARE_PENALTY_ENEMY_H

#include <vector>
#include "ai/role/role.h"
#include "ai/tactic/move.h"

//
// Gets the robots to go to their penalty_enemy positions.
//
class penalty_enemy : public role {
	public:
		//
		// A pointer to a penalty_enemy role.
		//
		typedef Glib::RefPtr<penalty_enemy> ptr;

		//
		// Constructs a new penalty_enemy role.
		//
		penalty_enemy(world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		void robots_changed();

	protected:
		const world::ptr the_world;

		/**
		 * The distance between the baseline and the line behind which the robots must stand.
		 */
		const static double RESTRICTED_ZONE_LENGTH = 0.85;

		/**
		* Maximum number of positions that can be assigned for this role.
		*/
		const static unsigned int NUM_POSITIONS = 4;

		/**
		* The designated standing positions for this role.
		*/
		point standing_positions[NUM_POSITIONS];
};

class penalty_goalie : public role {
	public:
		typedef Glib::RefPtr<penalty_goalie> ptr;

		penalty_goalie(world::ptr world);

		void tick();

		void robots_changed();

	protected:
		const world::ptr the_world;
};

#endif

