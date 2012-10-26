#ifndef AI_HL_STP_ENEMY_H
#define AI_HL_STP_ENEMY_H

#include "ai/hl/stp/world.h"
#include "util/noncopyable.h"
#include <memory>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * \brief Describes an enemy.
			 *
			 * Allows tactics to dynamically target enemies.
			 * E.g. you want to block the closest enemy.
			 */
			class Enemy : public NonCopyable {
				public:
					/**
					 * \brief A pointer to an Enemy.
					 */
					typedef std::shared_ptr<Enemy> Ptr;

					/**
					 * \brief Returns the enemy robot associated with this role.
					 *
					 * \return the enemy robot, or null if such a robot does not exist.
					 */
					virtual Robot evaluate() const = 0;

					/**
					 * \brief Order by distance to friendly goal.
					 *
					 * \param[in] world the World in which STP operates
					 *
					 * \param[in] i the index of the enemy ordered by distance to friendly goal
					 * where 0 is closest and 1 is second closest etc.
					 *
					 * \return the (i+1)th closest enemy to the friendly goal
					 */
					static Enemy::Ptr closest_friendly_goal(World world, unsigned int i);

					/**
					 * \brief Order by distance to ball.
					 *
					 * \param[in] world the World in which STP operates
					 *
					 * \param[in] i the index of the enemy ordered by distance to ball
					 * where 0 is closest and 1 is second closest etc.
					 *
					 * \return the (i+1)th closest enemy to the ball
					 */
					static Enemy::Ptr closest_ball(World world, unsigned int i);

					/**
					 * \brief Closest to enemy having the ball, by passing.
					 */
					static Enemy::Ptr closest_pass(World world, unsigned int i);

					/**
					 * \brief Order by distance to a friendly player.
					 *
					 * \param[in] world the World in which STP operates
					 *
					 * \param[in] i the index of the enemy ordered by distance to friendly player
					 * where 0 is closest and 1 is second closest etc.
					 *
					 * \return the (i+1)th closest enemy to the friendly player
					 */
					static Enemy::Ptr closest_friendly_player(World world, Player player, unsigned int i);

				protected:
					Enemy();
			};
		}
	}
}

#endif

