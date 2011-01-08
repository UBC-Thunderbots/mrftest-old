#ifndef AI_HL_STP_TACTIC_DEFEND_H
#define AI_HL_STP_TACTIC_DEFEND_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * A standard lone goalie tactic.
				 */
				Tactic::Ptr defend_goal(AI::HL::W::World &world);

				/**
				 * Move the ball away from our own goal at all cost.
				 */
				Tactic::Ptr repel(AI::HL::W::World &world);
			}
		}
	}
}

#endif

