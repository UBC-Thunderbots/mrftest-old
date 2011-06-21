#ifndef AI_HL_OLD_UTIL_H
#define AI_HL_OLD_UTIL_H

#include "ai/hl/world.h"
#include "util/param.h"
#include <vector>

namespace AI {
	namespace HL {
		namespace Util {
			/**
			 * Finds the best assignment of players to points.
			 * This is done by computing a minimum-total-distance bipartite matching between the sets of points.
			 * This function should work even if the number of players and waypoints do not match.
			 *
			 * \param[in] players a list of players.
			 *
			 * \param[in, out] waypoints a list of points in which to assign the players, which will be reordered.
			 */
			void waypoints_matching(const std::vector<AI::HL::W::Player::Ptr> &players, std::vector<Point> &waypoints);

			/**
			 * Finds the best player to pass to.
			 * This player must:
			 * - see a significant portion of the enemy goal
			 * - able to receive the ball
			 *
			 * \param[in] friends the list of players to pass to.
			 */
			AI::HL::W::Player::Ptr choose_best_pass(AI::HL::W::World &world, const std::vector<AI::HL::W::Player::Ptr> &friends);
		}
	}
}

#endif

