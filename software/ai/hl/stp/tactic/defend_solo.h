#ifndef AI_HL_STP_TACTIC_DEFEND_SOLO_H
#define AI_HL_STP_TACTIC_DEFEND_SOLO_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Used for single goalie with NO defenders.
				 */
				Tactic::Ptr defend_solo_goalie(const World &world);

				/**
				 * Used for single goalie with NO defenders and is an active tactic.
				 */
				Tactic::Ptr active_solo_goalie(const World &world);
			}
		}
	}
}

#endif

