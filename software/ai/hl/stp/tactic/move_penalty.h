#ifndef AI_HL_STP_TACTIC_MOVE_PENALTY_H
#define AI_HL_STP_TACTIC_MOVE_PENALTY_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Move to a penalty location.
				 */
				Tactic::Ptr move_penalty(AI::HL::W::World &world, const Coordinate dest);
			}
		}
	}
}

#endif

