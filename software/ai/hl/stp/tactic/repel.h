#ifndef AI_HL_STP_TACTIC_REPEL_H
#define AI_HL_STP_TACTIC_REPEL_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Repel
				 * Active Tactic
				 * Move the ball away from our own goal at all cost.
				 * Active tactic.
				 */
				Tactic::Ptr repel(AI::HL::W::World world);

				/**
				 * Corner Repel
				 * Active Tactic
				 * Special repel to be used in corner kicks
				 * Active tactic.
				 */
				Tactic::Ptr corner_repel(AI::HL::W::World world);
			}
		}
	}
}


#endif

