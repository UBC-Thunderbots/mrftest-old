#ifndef AI_HL_STP_EVALUATION_DEFENSE_H
#define AI_HL_STP_EVALUATION_DEFENSE_H

#include "ai/hl/stp/world.h"

#include <array>

namespace {
	const int MAX_DEFENDERS = 3;
}

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {

				/**
				 * Locations:
				 * 0 - goalie
				 * 1 - defender
				 * 2 - defender-extra1
				 * 3 - defender-extra2
				 */
				const std::array<Point, MAX_DEFENDERS + 1> evaluate_defense(const World &world);
			}
		}
	}
}

#endif

