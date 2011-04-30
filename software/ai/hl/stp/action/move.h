#ifndef AI_HL_STP_ACTION_MOVE_H
#define AI_HL_STP_ACTION_MOVE_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move to a particular location and stop.
				 */
				void move(const World& world, Player::Ptr player, const Point dest);
			}
		}
	}
}

#endif

