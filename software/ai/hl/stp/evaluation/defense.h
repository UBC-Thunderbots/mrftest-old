#ifndef AI_HL_STP_EVALUATION_DEFENSE_H
#define AI_HL_STP_EVALUATION_DEFENSE_H

#include "ai/hl/world.h"

#include <array>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Computes ideal locations to place a goalie + 1 defender
				 * N is the team size.
				 */
				template<int N> const std::array<Point, N> evaluate_defense(const AI::HL::W::World& world);
			}
		}
	}
}

#endif

