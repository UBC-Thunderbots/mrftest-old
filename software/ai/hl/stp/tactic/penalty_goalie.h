#ifndef AI_HL_STP_TACTIC_PENALTY_GOALIE_H
#define AI_HL_STP_TACTIC_PENALTY_GOALIE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Only to be used for defending against penalty kicks.
				 * Active Tactic.
				 */
				Tactic::Ptr penalty_goalie(const World &world);
			}
		}
	}
}

#endif

