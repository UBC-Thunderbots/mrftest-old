#ifndef AI_HL_STP_EVALUATE_ENEMY_H
#define AI_HL_STP_EVALUATE_ENEMY_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluate {
				/**
				 * Describes a way to describe enemy role.
				 * The STP paper calls it the criterion, see section 5.2.4.
				 */
				enum EnemyOrdering {
					/**
					 * Orders enemy by distance to own goal.
					 */
					ENEMY_ORDER_DIST_FRIENDLY_GOAL,

					/**
					 * Orders enemy by distance to enemy goal.
					 */
					ENEMY_ORDER_DIST_ENEMY_GOAL,

					/**
					 * Orders enemy by distance to the ball.
					 */
					ENEMY_ORDER_DIST_BALL,
				};

				/**
				 * Describes an enemy role.
				 */
				class EnemyRole : public ByRef {
					public:
						typedef RefPtr<EnemyRole> Ptr;

						virtual AI::HL::W::Robot::Ptr evaluate(AI::HL::W::World& world) const = 0;

						static EnemyRole::Ptr closest_friendly_goal(int i);

						static EnemyRole::Ptr closest_enemy_goal(int i);

						static EnemyRole::Ptr closest_ball(int i);

					protected:
						EnemyRole(EnemyOrdering o, int i);

						EnemyOrdering ordering_;
						int index_;
				};
			}
		}
	}
}

#endif

