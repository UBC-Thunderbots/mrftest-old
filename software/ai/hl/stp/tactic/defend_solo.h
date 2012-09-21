#ifndef AI_HL_STP_TACTIC_DEFEND_SOLO_H
#define AI_HL_STP_TACTIC_DEFEND_SOLO_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Used for single goalie with NO defenders.
				 * USE IT ONLY IF YOU ARE SURE OF WHAT YOU ARE DOING
				 */
				Tactic::Ptr lone_goalie(World world);

				/**
				 * Used for single goalie with NO defenders and is an active tactic.
				 */
				Tactic::Ptr lone_goalie_active(World world);
			}
		}
	}
}

#endif

