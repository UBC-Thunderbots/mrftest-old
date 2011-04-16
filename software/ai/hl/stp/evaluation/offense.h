#ifndef AI_HL_STP_EVALUATION_OFFENSE_H
#define AI_HL_STP_EVALUATION_OFFENSE_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "util/cacheable.h"
#include "util/param.h"

#include <array>
#include <set>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * General purpose scoring function for offensive positions.
				 *
				 * \param[in] pos the position to calculate the score for.
				 *
				 * \return a score for the location. This score has no range limit.
				 */
				double offense_score(const World &world, const Point pos);

				/**
				 * Calculates strategic positions to place offensive players.
				 * - Finds weak points on the enemy goal area,
				 *   where the enemy net is open and the ball is visible.
				 * - Places that are near players are preferred.
				 * - Additionally, if some other player is visible to the goal area,
				 *   will avoid blocking view of it.
				 *
				 * About offense evaluation:
				 * - there can only be up to 2 players doing offense: primary and secondary
				 * - the secondary offender must not block the primary offender
				 * - ideally, the secondary offender is as far away from the primary offender as possible
				 *   note that the calculation of the secondary offender depends on the primary offender
				 * - if no valid position is found for a particular
				 */
				std::array<Point, 2> offense_positions(const World& world);

				/**
				 * Grid size in x-direction.
				 */
				extern IntParam grid_x;

				/**
				 * Grid size in y-direction.
				 */
				extern IntParam grid_y;
			}
		}
	}
}

#endif

