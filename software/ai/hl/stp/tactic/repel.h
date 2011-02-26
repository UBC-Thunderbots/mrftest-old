#ifndef AI_HL_STP_TACTIC_REPEL_H
#define AI_HL_STP_TACTIC_REPEL_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Move the ball away from our own goal at all cost.
				 * Active tactic.
				 */
				Tactic::Ptr repel(const AI::HL::W::World &world);
			}
		}
	}
}


#endif

