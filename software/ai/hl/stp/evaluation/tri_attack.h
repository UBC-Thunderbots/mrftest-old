#ifndef AI_HL_STP_EVALUATION_TRI_ATTACK_H
#define AI_HL_STP_EVALUATION_TRI_ATTACK_H

#include "ai/hl/stp/world.h"

#include <array>

namespace {
	const unsigned int MAX_ATTACKERS = 4;
}

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Locations:
				 * 0 - active tactic attacker
				 * 1 - attacker secondary
				 * 2 - attacker tertiary
				 * 3 - attacker-extra (for diamond formation)
				 */
				const std::array<Point, MAX_ATTACKERS> evaluate_attack();

				/**
				 * return point to where player needs to go in a tri-attack
				 */
				Point tri_attack_evaluation(World world);
			}
		}
	}
}

#endif
