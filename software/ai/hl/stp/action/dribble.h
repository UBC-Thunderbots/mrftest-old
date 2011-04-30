#ifndef AI_HL_STP_ACTION_DRIBBLE_H
#define AI_HL_STP_ACTION_DRIBBLE_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Dribble to a particular location and stop.
				 */
				void dribble(const World& world, Player::Ptr player, const Point dest);

				/**
				 * Dribble and stay at the same position.
				 */
				void dribble(const World& world, Player::Ptr player);
			}
		}
	}
}

#endif

