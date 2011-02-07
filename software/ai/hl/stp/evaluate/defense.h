#ifndef AI_HL_STP_EVALUATE_DEFENSE_H
#define AI_HL_STP_EVALUATE_DEFENSE_H

#include "ai/hl/world.h"
#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Calculates the best position for up to 2 defenders and 1 goalie.
				 * For this class to function,
				 * it needs at least one goalie and one defender.
				 */
				std::vector<Point> evaluate_defense(AI::HL::W::World& world);
			}
		}
	}
}

#endif

