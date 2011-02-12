#ifndef AI_HL_STP_TACTIC_STEAL_H
#define AI_HL_STP_TACTIC_STEAL_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Steal the goal from the enemy who has the ball in the world.
				 */
				Tactic::Ptr steal(AI::HL::W::World &world);

			}
		}
	}
}

#endif

