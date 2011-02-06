#ifndef AI_HL_STP_EVALUATE_OFFENSE_H
#define AI_HL_STP_EVALUATE_OFFENSE_H

#include "ai/hl/world.h"
#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Calculates strategic positions to place offensive players.
				 * Finds weak points on the enemy goal area,
				 * where the enemy net is open and the ball is visible.
				 *
				 * \param[in] preferred points nearer to this location is preferred.
				 */
				Point evaluate_offense(AI::HL::W::World& world, Point preferred);
			}
		}
	}
}

#endif

