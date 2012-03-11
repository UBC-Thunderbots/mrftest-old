#ifndef AI_HL_STP_ACTION_CHASE_H
#define AI_HL_STP_ACTION_CHASE_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Chases after the ball as fast as possible.
				 * Orient towards Point target.
				 */
				void chase(Player::Ptr player, Point target);
			}
		}
	}
}

#endif

