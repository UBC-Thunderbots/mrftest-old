#ifndef AI_HL_STP_EVALUATION_OFFENSE_H
#define AI_HL_STP_EVALUATION_OFFENSE_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "util/cacheable.h"

#include <set>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/*
				   class EvaluateOffense : public Cacheable<Point, CacheableNonKeyArgs<AI::HL::W::World &>, CacheableKeyArgs<const std::set<AI::HL::W::Player::Ptr> &> > {
				   protected:
				   Point compute(AI::HL::W::World &world, const std::set<AI::HL::W::Player::Ptr> & players);
				   };

				   extern EvaluateOffense evaluate_offense2;
				 */

				/**
				 * Calculates strategic positions to place offensive players.
				 * - Finds weak points on the enemy goal area,
				 *   where the enemy net is open and the ball is visible.
				 * - Places that are near players are preferred.
				 * - Additionally, if some other player is visible to the goal area,
				 *   will avoid blocking view of it.
				 *
				 * \param[in] players the set of players that influence the output.
				 */
				Point evaluate_offense(const AI::HL::W::World &world, const std::set<AI::HL::W::Player::Ptr> &players);
			}
		}
	}
}

#endif

