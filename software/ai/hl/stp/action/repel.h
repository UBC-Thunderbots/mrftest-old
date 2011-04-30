#ifndef AI_HL_STP_ACTION_REPEL_H
#define AI_HL_STP_ACTION_REPEL_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move the ball as far away as possible from friendly goal.
				 * Useful scenarios:
				 * - ball rolling towards the friendly goal
				 * - ball inside the defense area
				 */
				void repel(const World &world, Player::Ptr player);
			}
		}
	}
}

#endif

