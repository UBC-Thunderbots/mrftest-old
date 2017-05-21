#pragma once

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/action/action.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Repel
				 *
				 * Not intended for goalie use
				 *
				 * Move the ball as far away as possible from friendly goal.
				 * Useful scenarios:
				 * - ball rolling towards the friendly goal
				 * - ball inside the defense area
				 *
				 * \return true if kicked
				 */
				bool repel(caller_t& ca, World world, Player player);

				/**
				 * Corner Repel
				 *
				 * Not intended for goalie use
				 *
				 * Repel in the corner
				 */
				bool corner_repel(caller_t& ca, World world, Player player);

				/**
				 * Goalie Repel
				 *
				 * Intended for goalie use
				 *
				 * Repel as the goalie, take our time when we have the ball
				 */

				bool goalie_repel(caller_t& ca, World world, Player player);
			}
		}
	}
}
