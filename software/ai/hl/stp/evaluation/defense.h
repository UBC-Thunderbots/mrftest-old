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

				void tick_defense(const World& world);

				/**
				 * Locations:
				 * 0 - goalie
				 * 1 - defender
				 * 2 - defender-extra1
				 * 3 - defender-extra2
				 */
				const std::array<Point, MAX_DEFENDERS + 1> evaluate_defense();

				/**
				 * Checks if an enemy breaks our defense duo.
				 */
				bool enemy_break_defense_duo(const World& world, const Robot::Ptr enemy);
				
				/**
				 * return dest of Terence defender - index.
				 */
				Point evaluate_tdefense(const World &world, const unsigned index);
			}
		}
	}
}

#endif

