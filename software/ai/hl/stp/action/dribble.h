#ifndef AI_HL_STP_ACTION_DRIBBLE_H
#define AI_HL_STP_ACTION_DRIBBLE_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Dribble
				 *
				 * Not intended for goalie use
				 *
				 * Dribble and stay at the same position.
				 */
				void dribble(Player player);

				/**
				 * Dribble
				 *
				 * Not intended for goalie use
				 *
				 * Dribble to a particular location and stop.
				 */
				void dribble(World world, Player player, const Point dest);
			}
		}
	}
}

#endif

