#ifndef AI_HL_STP_ENEMY_H
#define AI_HL_STP_ENEMY_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Describes an enemy.
			 * Allows tactics to dynamically target enemies.
			 * E.g. you want to block the closest enemy.
			 */
			class Enemy : public ByRef {
				public:
					typedef RefPtr<Enemy> Ptr;

					/**
					 * Returns the enemy robot associated with this role.
					 * If the robot does not exist,
					 * then a null pointer is returned.
					 */
					virtual Robot::Ptr evaluate() const = 0;

					/**
					 * Order by distance to friendly goal.
					 */
					static Enemy::Ptr closest_friendly_goal(const World &world, unsigned int i);

					/**
					 * Order by distance to ball.
					 */
					static Enemy::Ptr closest_ball(const World &world, unsigned int i);
					
					/**
					 * Order by distance to friendly goal.
					 */
					static Enemy::Ptr closest_pass(const World &world, const Robot::Ptr r, unsigned int i);


				protected:
					Enemy();
			};
		}
	}
}

#endif

