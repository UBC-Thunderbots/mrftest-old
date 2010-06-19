#ifndef AI_ROLE_DEFENSIVE2_H
#define AI_ROLE_DEFENSIVE2_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

/**
 * Combined goalie and defender
 * The first robot is defined to be the goalkeeper by default.
 * The goal is to completely block the goalpost.
 */
class defensive2 : public role {
	public:
		typedef Glib::RefPtr<defensive2> ptr;

		defensive2(world::ptr world);

		void tick();

		void robots_changed();

		/**
		 * Specifically chooses this player to be the goalie.
		 */
		void set_goalie(player::ptr p) {
			goalie = p;
		}

	protected:

		/**
		 * Calculate points which should be used to defend
		 * from enemy robots.
		 * Should be adjusted to take into account of the rules
		 * and number of robots in this role.
		 * Returns a pair
		 * - goalie position
		 * - other robots position
		 * Note:
		 * - if any of the position == ball position,
		 *   please use chase/shoot etc
		 */
		std::pair<point, std::vector<point> > calc_block_positions(const bool top) const;

		void assign(const player::ptr& p, tactic::ptr t);

		const world::ptr the_world;

		player::ptr goalie;
		std::vector<tactic::ptr> tactics;

};

#endif

