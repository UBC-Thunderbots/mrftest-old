#ifndef AI_ROLE_DEFENSIVE2_H
#define AI_ROLE_DEFENSIVE2_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

/**
 * Combined goalie and defender
 * The first robot is defined to be the goalkeeper.
 * The goal is to completely block the goalpost.
 */
class defensive2 : public role {
	public:
		//
		// A pointer to a defensive role.
		//
		typedef Glib::RefPtr<defensive2> ptr;

		//
		// Constructs a new defensive role.
		//
		defensive2(world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:

		/**
		 * Calculate points which should be used to defend from enemy robots.
		 * Should be adjusted to take into account of the rules and number of robots in this role.
		 */
		std::vector<point> calc_block_positions(const bool top = true) const;

		const world::ptr the_world;

		std::vector<tactic::ptr> tactics;

		point calc_goalie_pos(const bool top) const;
};

#endif

