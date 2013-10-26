#ifndef AI_HL_STP_TACTIC_SHOOT_H
#define AI_HL_STP_TACTIC_SHOOT_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Shoot Goal
				 * Active Tactic
				 * Shoot for the enemy goal.
				 */
				Tactic::Ptr shoot_goal(World world, bool force = false);

				/**
				 * Shoot Target
				 * Active Tactic
				 * Shoot a specified target.
				 */
				Tactic::Ptr shoot_target(World world, const Coordinate target);
			}
		}
	}
}

#endif

