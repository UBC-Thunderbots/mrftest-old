#ifndef AI_HL_STP_ACTION_CHIP_H
#define AI_HL_STP_ACTION_CHIP_H

#include "ai/hl/stp/world.h"

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
				bool chip_target(World world, Player player, const Point target, double power = 0.6);
				
			}
		}
	}
}

#endif

