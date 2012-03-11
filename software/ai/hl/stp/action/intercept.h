#ifndef AI_HL_STP_ACTION_INTERCEPT_H
#define AI_HL_STP_ACTION_INTERCEPT_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Intercept the ball facing towards the target.
				 *
				 *  \param[in] target Used to determine the direction the player should be facing when player intercepts the ball.
				 */
				void intercept(Player::Ptr player, const Point target);
			}
		}
	}
}

#endif
