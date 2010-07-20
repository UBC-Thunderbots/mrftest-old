#ifndef AI_DEFENSIVE3_H
#define AI_DEFENSIVE3_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include <vector>
#include <set>

/**
 * Combined goalie and defender
 * The goalie is always automatically assigned,
 * and never changes unless removed from this role.
 *
 * NEW DEFENSIVE for STATEFUL AI
 * This supercedes all previous defensive role implementations
 *
 * Notes
 * - perhaps add_player/remove_player make more sense than set_players
 *   since we are moving to stateful AI
 * - must not have more defenders than 1 + number of enemy players
 */
class Defensive3 {
	public:

		Defensive3(const World::ptr world);

		void tick();

		void add_player(Player::ptr player);

		void remove_player(Player::ptr player);

		void clear_players();

		/**
		 * Sets the goalie manually.
		 * Will add player to this role.
		 */
		void set_goalie(Player::ptr player);

		/**
		 * Chooses and removes one player from this role.
		 * Ideally the least important player.
		 */
		Player::ptr pop_player();

	protected:

		/**
		 * Calculate points which should be used to defend from enemy robots.
		 * Should be adjusted to take into account of the rules
		 * and number of robots in this Role.
		 * Returns a pair
		 * - goalie position
		 * - 4 other robots position, ordered from the most important
		 * Note:
		 * - if any of the position == ball position, implies Chase/Shoot etc
		 */
		std::pair<Point, std::vector<Point> > calc_block_positions(const bool top) const;

		const World::ptr world;

		std::set<Player::ptr> players;

		Player::ptr goalie;

		/// Should goalie defend the top part of the net?
		bool goalie_guard_top;
};

#endif

