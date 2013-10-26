#ifndef AI_HL_STP_TACTIC_PENALTY_SHOOT_H
#define AI_HL_STP_TACTIC_PENALTY_SHOOT_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Penalty Shoot
				 * Active Tactic
				 * Shoot for the enemy goal with the shoot goal tactic.
				 */
				Tactic::Ptr penalty_shoot(AI::HL::W::World world);
			}
		}
	}
}

#endif

