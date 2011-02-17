#ifndef AI_HL_STP_TACTIC_DEFEND_H
#define AI_HL_STP_TACTIC_DEFEND_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Used for single goalie with NO defenders.
				 */
				Tactic::Ptr defend_solo_goalie(AI::HL::W::World &world);

				/**
				 * Used for goalie + AT LEAST 1 defender.
				 * A tactic for goalie.
				 */
				Tactic::Ptr defend_duo_goalie(AI::HL::W::World &world);

				/**
				 * Used for goalie + AT LEAST 1 defender.
				 * A tactic for defender (MUST EXIST).
				 */
				Tactic::Ptr defend_duo_defender(AI::HL::W::World &world);

				/**
				 * Used for goalie + AT LEAST 1 defender.
				 * A tactic for extra player.
				 */
				Tactic::Ptr defend_duo_extra(AI::HL::W::World &world);
			}
		}
	}
}

#endif

