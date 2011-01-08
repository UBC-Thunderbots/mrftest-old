#ifndef AI_HL_STP_TACTIC_SHOOT_H
#define AI_HL_STP_TACTIC_SHOOT_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Shoot for the enemy goal.
				 */
				Tactic::Ptr shoot(AI::HL::W::World &world);

				/**
				 * Shoot a specified target.
				 */
				Tactic::Ptr shoot(AI::HL::W::World &world, const Point target);
			}
		}
	}
}

#endif

