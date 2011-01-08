#ifndef AI_HL_STP_TACTIC_MOVE_H
#define AI_HL_STP_TACTIC_MOVE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Move to a particular location.
				 */
				Tactic::Ptr move(AI::HL::W::World &world, const Point dest);
			}
		}
	}
}

#endif

