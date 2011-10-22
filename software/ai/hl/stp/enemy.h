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
					 *
					 * \param[in] world the World in which STP operates
					 *
					 * \param[in] i the index of the enemy ordered by distance to friendly goal
					 * where 0 is closest and 1 is second closest etc.
					 *
					 * \return the (i+1)th closest enemy to the friendly goal
					 */
					static Enemy::Ptr closest_friendly_goal(const World &world, unsigned int i);

					/**
					 * Order by distance to ball.
					 *
					 * \param[in] world the World in which STP operates
					 *
					 * \param[in] i the index of the enemy ordered by distance to ball
					 * where 0 is closest and 1 is second closest etc.
					 *
					 * \return the (i+1)th closest enemy to the ball
					 */
					static Enemy::Ptr closest_ball(const World &world, unsigned int i);

					/**
					 * Closest to enemy having the ball, by passing.
					 */
					static Enemy::Ptr closest_pass(const World &world, unsigned int i);

				protected:
					Enemy();
			};
		}
	}
}

#endif

