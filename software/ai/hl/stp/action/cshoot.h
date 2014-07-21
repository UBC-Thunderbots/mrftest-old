#ifndef AI_HL_STP_ACTION_CSHOOT_H
#define AI_HL_STP_ACTION_CSHOOT_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Shoots the ball to a target point with a double param kicking speed for passing
				 *
				 * \param[in] pass true if the player is to pass.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool cshoot_target(World world, Player player, const Point target, double velocity = BALL_MAX_SPEED);
			}
		}
	}
}

#endif

