#ifndef AI_HL_STP_TACTIC_CSHOOT_H
#define AI_HL_STP_TACTIC_CSHOOT_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * C-Shoot Goal
				 * Active Tactic
				 * Shoot for the enemy goal after repositioning within a circle (ask Kevin)
				 */
				Tactic::Ptr cshoot_goal(World world, bool force = false);
			}
		}
	}
}

#endif

