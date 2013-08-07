#ifndef AI_HL_STP_EVALUATION_DEFENSE_H
#define AI_HL_STP_EVALUATION_DEFENSE_H

#include "ai/hl/stp/world.h"

#include <array>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				constexpr unsigned int MAX_DEFENDERS = 3;

				void tick_defense(World world);

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
				bool enemy_break_defense_duo(World world, Robot enemy);

				/**
				 * return dest of Terence defender - index.
				 */
				Point evaluate_tdefense(World world, const unsigned index);
				
				/**
				 * return dest of Terence defender line.
				 */
				Point evaluate_tdefense_line(World world, const Point p1, const Point p2, const double dist_min, const double dist_max);
			}
		}
	}
}

#endif

