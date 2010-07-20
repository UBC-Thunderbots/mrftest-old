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
class Defensive2 : public Role {
	public:
		Defensive2(RefPtr<World> world);

		void tick();

		void players_changed();

		/**
		 * Specifically chooses this player to be the goalie.
		 */
		void set_goalie(RefPtr<Player> p) {
			goalie = p;
		}

	protected:

		/**
		 * Calculate points which should be used to defend
		 * from enemy robots.
		 * Should be adjusted to take into account of the rules
		 * and number of robots in this Role.
		 * Returns a pair
		 * - goalie position
		 * - other robots position
		 * Note:
		 * - if any of the position == ball position,
		 *   please use Chase/Shoot etc
		 */
		std::pair<Point, std::vector<Point> > calc_block_positions(const bool top) const;

		void assign(const RefPtr<Player>& p, RefPtr<Tactic> t);

		const RefPtr<World> the_world;

		RefPtr<Player> goalie;
		std::vector<RefPtr<Tactic> > tactics;

};

#endif

