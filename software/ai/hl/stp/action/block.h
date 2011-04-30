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
				void block(const World &world, Player::Ptr player, Robot::Ptr robot);

				/**
				 * Blocks against a single enemy from passing.
				 */
				void block_pass(const World &world, Player::Ptr player, Robot::Ptr robot);
			}
		}
	}
}

#endif

