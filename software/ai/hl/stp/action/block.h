#ifndef AI_HL_STP_ACTION_BLOCK_H
#define AI_HL_STP_ACTION_BLOCK_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Blocks against a single enemy from shooting to our goal.
				 */
				void block_goal(World world, Player player, Robot robot);

				/**
				 * Blocks against a single enemy from the ball / passing.
				 */
				void block_ball(World world, Player player, Robot robot);
			}
		}
	}
}

#endif

