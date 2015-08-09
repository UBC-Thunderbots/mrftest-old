#ifndef AI_HL_STP_ACTION_RAM_H
#define AI_HL_STP_ACTION_RAM_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Ram
				 *
				 * Not intended for goalie use
				 *
				 * Move to a particular location and stop. Orient the player
				 * towards the ball.
				 */
				void ram(World world, Player player, const Point dest);

				/**
				 * Ram
				 *
				 * Not intended for goalie use
				 *
				 * Ram defaulting to ram the ball
				 */
				void ram(World world, Player player);

				/**
				 * Ram
				 *
				 * Intended for goalie use
				 *
				 * Move to a particular location and stop. Orient the player
				 * towards the ball.
				 */
				void goalie_ram(World world, Player player, const Point dest);
			}
		}
	}
}

#endif

