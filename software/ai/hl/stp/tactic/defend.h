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
				Tactic::Ptr defend_duo_goalie(const World &world);

				/**
				 * Used for goalie + AT LEAST 1 defender.
				 * A tactic for defender (MUST EXIST).
				 */
				Tactic::Ptr defend_duo_defender(const World &world);

				/**
				 * Used for goalie + AT LEAST 1 defender.
				 * A tactic for extra player.
				 */
				Tactic::Ptr defend_duo_extra(const World &world);

				/**
				 * A goalie tactic that can switch between lone and duo goalie.
				 *
				 * \input[in] defender_role the role index (starting from 0) that the defender_duo_defender is.
				 */
				Tactic::Ptr goalie_dynamic(const World &world, const size_t defender_role);
			}
		}
	}
}

#endif

