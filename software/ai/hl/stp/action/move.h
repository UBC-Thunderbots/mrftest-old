#ifndef AI_HL_STP_ACTION_MOVE_H
#define AI_HL_STP_ACTION_MOVE_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move to a particular location and stop.
				 * Orient the player towards the ball.
				 */
				void move(const World& world, Player::Ptr player, const Point dest);

				/**
				 * Move to a particular location and stop.
				 * Orient the player towards a particular location.
				 */
				void move(Player::Ptr player, const double orientation, const Point dest);
			}
		}
	}
}

#endif

