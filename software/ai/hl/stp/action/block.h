#ifndef AI_HL_STP_ACTION_BLOCK_H
#define AI_HL_STP_ACTION_BLOCK_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Block Goal
				 *
				 * Not intended for goalie use
				 *
				 * Blocks against a single enemy from shooting to our goal.
				 */
				void block_goal(World world, Player player, Robot robot);

				/**
				 * Block Ball
				 *
				 * Not intended for goalie use
				 *
				 * Blocks against a single enemy from the ball / passing.
				 */
				void block_ball(World world, Player player, Robot robot);
			}
		}
	}
}

#endif

