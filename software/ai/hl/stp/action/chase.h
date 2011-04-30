#ifndef AI_HL_STP_ACTION_CHASE_H
#define AI_HL_STP_ACTION_CHASE_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * TODO: implement steal ball
				 *
				 * Chases after the ball as fast as possible.
				 * Orient towards the ball.
				 */
				void chase(const World &world, Player::Ptr player);
				
				/**
				 * Chases after the ball as fast as possible. Orient towards Point target.
				 */
				void chase(const World &world, Player::Ptr player, Point target);
			}
		}
	}
}

#endif

