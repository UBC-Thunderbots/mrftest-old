#ifndef AI_HL_STP_TACTIC_MOVE_H
#define AI_HL_STP_TACTIC_MOVE_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/evaluate/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Move to a particular location.
				 */
				Tactic::Ptr move(AI::HL::W::World &world, const Point dest);

				/**
				 * Move to a dynamic location.
				 */
				Tactic::Ptr move(AI::HL::W::World &world, const AI::HL::STP::Evaluation::Coordinate dest);
			}
		}
	}
}

#endif

