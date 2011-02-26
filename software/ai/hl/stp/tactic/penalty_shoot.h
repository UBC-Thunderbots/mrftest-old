#ifndef AI_HL_STP_TACTIC_PENALTY_SHOOT_H
#define AI_HL_STP_TACTIC_PENALTY_SHOOT_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Shoot for the enemy goal.
				 */
				Tactic::Ptr penalty_shoot(const AI::HL::W::World &world);
			}
		}
	}
}

#endif

