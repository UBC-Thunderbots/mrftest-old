#ifndef AI_HL_STP_EVALUATION_DEFENSE_H
#define AI_HL_STP_EVALUATION_DEFENSE_H

#include "ai/hl/stp/world.h"

#include <array>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Computes locations to place 1 goalie and EXACTLY 1 OR 2 defenders.
				 */
				const std::array<Point, 3> evaluate_defense(const World& world);
			}
		}
	}
}

#endif

