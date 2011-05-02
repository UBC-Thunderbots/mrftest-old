#ifndef AI_HL_STP_TACTIC_PASS_H
#define AI_HL_STP_TACTIC_PASS_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				
				/**
				 * Passer positioning and shoot for offensive purposes
				 * (objective is pass to passee and have it shoot at enemy goal).
				 */
				Tactic::Ptr passer_shoot(const World &world);

				/**
				 * Passee positioning for offensive.
				 */
				Tactic::Ptr passee_receive(const World &world);
				
				/**
				 * Passer positioning and shoot for defensive purposes
				 * (objective is to keep ball in team possesion).
				 */
				Tactic::Ptr def_passer_shoot(const World &world);
				
				/**
				 * Passee positioning for defensive.
				 */
				Tactic::Ptr def_passee_receive(const World &world);
			}
		}
	}
}

#endif

