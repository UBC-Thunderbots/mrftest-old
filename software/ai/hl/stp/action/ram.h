#ifndef AI_HL_STP_ACTION_RAM_H
#define AI_HL_STP_ACTION_RAM_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move to a particular location and stop.
				 * Orient the player towards the ball.
				 */
				void ram(const World &world, Player::Ptr player, const Point dest, const Point vel);

				/**
				 * ram defaulting to ram the ball
				 */
				void ram(const World &world, Player::Ptr player);
			}
		}
	}
}

#endif

