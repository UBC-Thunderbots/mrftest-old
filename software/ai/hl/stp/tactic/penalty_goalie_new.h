#ifndef AI_HL_STP_TACTIC_PENALTY_GOALIE_NEW_H
#define AI_HL_STP_TACTIC_PENALTY_GOALIE_NEW_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Penalty Goalie
				 * Active Tactic (But it's never done)
				 * Only to be used for defending against penalty kicks.
				 */
				Tactic::Ptr penalty_goalie_new(World world);
			}
		}
	}
}

#endif

