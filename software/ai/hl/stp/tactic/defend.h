#ifndef AI_HL_STP_TACTIC_DEFEND_H
#define AI_HL_STP_TACTIC_DEFEND_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/tactic/defend_solo.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Used for goalie + AT LEAST 1 defender.
				 * A tactic for goalie.
				 */
				Tactic::Ptr defend_duo_goalie(const AI::HL::W::World &world);

				/**
				 * Used for goalie + AT LEAST 1 defender.
				 * A tactic for defender (MUST EXIST).
				 */
				Tactic::Ptr defend_duo_defender(const AI::HL::W::World &world);

				/**
				 * Used for goalie + AT LEAST 1 defender.
				 * A tactic for extra player.
				 */
				Tactic::Ptr defend_duo_extra(const AI::HL::W::World &world);
			}
		}
	}
}

#endif

