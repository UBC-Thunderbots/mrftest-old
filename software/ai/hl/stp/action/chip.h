#ifndef AI_HL_STP_ACTION_SHOOT_H
#define AI_HL_STP_ACTION_SHOOT_H

#include "ai/hl/stp/world.h"
#include "geom/rect.h"
#include "util/param.h"
#include "ai/hl/util.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {

				/**
				 * 
				 * Chips the ball to a target point with a double param power indicating the power to chip.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool chip_target(World world, Player::Ptr player, const Point target, double power = 1.0);
				
			}
		}
	}
}

#endif

