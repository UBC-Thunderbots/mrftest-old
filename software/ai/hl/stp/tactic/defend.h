#ifndef AI_HL_STP_TACTIC_DEFEND_H
#define AI_HL_STP_TACTIC_DEFEND_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/evaluation/defense.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * A tactic for goalie.
				 */
				Tactic::Ptr goalie(AI::HL::W::World& world, const AI::HL::STP::Evaluation::Defense& defense);

				/**
				 * A tactic for first defender.
				 */
				Tactic::Ptr defender1(AI::HL::W::World& world, const AI::HL::STP::Evaluation::Defense& defense);

				/**
				 * A tactic for second defender.
				 */
				Tactic::Ptr defender2(AI::HL::W::World& world, const AI::HL::STP::Evaluation::Defense& defense);
			}
		}
	}
}

#endif

