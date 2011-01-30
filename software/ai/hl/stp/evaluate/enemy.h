#ifndef AI_HL_STP_EVALUATE_ENEMY_H
#define AI_HL_STP_EVALUATE_ENEMY_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluate {
				/**
				 * Describes an enemy role.
				 * Allows tactics to dynamically target enemies.
				 */
				class EnemyRole : public ByRef {
					public:
						typedef RefPtr<EnemyRole> Ptr;

						EnemyRole();

						~EnemyRole();

						/**
						 * Returns the enemy robot associated with this role.
						 * If the robot does not exist,
						 * then a null pointer is returned.
						 */
						virtual AI::HL::W::Robot::Ptr evaluate() const = 0;

						/**
						 * Fixed role.
						 * Be very sure if you ever want to use this.
						 */
						static EnemyRole::Ptr fixed(AI::HL::W::Robot::Ptr robot);

						/**
						 * Order by distance to friendly goal.
						 */
						static EnemyRole::Ptr closest_friendly_goal(AI::HL::W::World& world, unsigned int i);
				};
			}
		}
	}
}

#endif

