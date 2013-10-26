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
				 * Active Tactic
				 * Takes the ball and dribbles towards a determined safe point 
				 */
				Tactic::Ptr tri_attack_primary(AI::HL::W::World world);
				/**
				 * Tri Attach Secondary
				 * Not active tactic
				 * Defend a line
				 * If p1_ == p2_ it'll be equivalent as defending a point
				 */
				Tactic::Ptr tri_attack_secondary(AI::HL::W::World world);
				
				/** 
				 * Tri Attack Tertiary
				 * Not active Tactic
				 * Moves towards a determined safe point. 
				 */
				Tactic::Ptr tri_attack_tertiary(AI::HL::W::World world);

			}
		}
	}
}

#endif
