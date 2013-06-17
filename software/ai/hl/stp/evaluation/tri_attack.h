#ifndef AI_HL_STP_EVALUATION_TRI_ATTACK_H
#define AI_HL_STP_EVALUATION_TRI_ATTACK_H

#include "ai/hl/stp/world.h"

#include <array>
#include <utility>

namespace {
	// not counting the active tactic
	const unsigned int MAX_ATTACKERS = 3;
}

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				void tick_tri_attack(World world);

				/**
				 * Locations:
				 * 0 - attacker secondary
				 * 1 - attacker tertiary
				 * 2 - attacker-extra (for diamond formation)
				 */
				const std::array<std::pair<Point, Point>, MAX_ATTACKERS> evaluate_tri_attack();

				/**
				 * return point to where player needs to go in a tri-attack
				 */
				Point tri_attack_evaluation(World world);
			}
		}
	}
}

#endif
