#ifndef AI_HL_STP_TACTIC_LONE_GOALIE_H
#define AI_HL_STP_TACTIC_LONE_GOALIE_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/evaluation/defense.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * A standard lone goalie tactic.
				 */
				Tactic::Ptr lone_goalie(const AI::HL::W::World &world);
			}
		}
	}
}

#endif

