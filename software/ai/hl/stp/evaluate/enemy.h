#ifndef AI_HL_STP_EVALUATE_ENEMY_H
#define AI_HL_STP_EVALUATE_ENEMY_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Describes an enemy role.
				 * Allows tactics to dynamically target enemies.
				 * E.g. you want to block the closest enemy.
				 */
				class EnemyRole : public ByRef {
					public:
						typedef RefPtr<EnemyRole> Ptr;

						/**
						 * Returns the enemy robot associated with this role.
						 * If the robot does not exist,
						 * then a null pointer is returned.
						 */
						virtual AI::HL::W::Robot::Ptr evaluate() const = 0;

						/**
						 * Checks whether the enemy exist or not.
						 */
						bool exist() const {
							return evaluate().is();
						}

						/**
						 * Fixed role.
						 * Be very sure if you ever want to use this.
						 */
						static EnemyRole::Ptr fixed(AI::HL::W::Robot::Ptr robot);

						/**
						 * Order by distance to friendly goal.
						 */
						static EnemyRole::Ptr closest_friendly_goal(AI::HL::W::World& world, unsigned int i);

						/**
						 * Order by distance to ball.
						 */
						static EnemyRole::Ptr closest_ball(AI::HL::W::World& world, unsigned int i);
					protected:
						EnemyRole();

						~EnemyRole();
				};
			}
		}
	}
}

#endif

