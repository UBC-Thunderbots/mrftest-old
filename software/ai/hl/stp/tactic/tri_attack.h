#ifndef AI_HL_STP_TACTIC_TRI_ATTACK_H
#define AI_HL_STP_TACTIC_TRI_ATTACK_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Tri_attack offence
				 */
				Tactic::Ptr tri_attack_active(AI::HL::W::World world);
				/**
				 * Defend a line
				 * If p1_ == p2_ it'll be equivalent as defending a point
				 */
				Tactic::Ptr tri_attack_secondaries(AI::HL::W::World world);
				
				Tactic::Ptr tri_attack_tertiary(AI::HL::W::World world);

			}
		}
	}
}

#endif

