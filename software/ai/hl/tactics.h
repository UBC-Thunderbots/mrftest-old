#ifndef AI_HL_TACTICS_H
#define AI_HL_TACTICS_H

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		/**
		 * Tactics are helper methods/classes which allow strategies
		 * to compose commonly used complex movement.
		 *
		 * It also serves to unify definition.
		 *
		 * Hint:
		 * - It is much easier to change the implementation of a tactic here,
		 * instead of changing every usage of the player movement.
		 *
		 */
		namespace Tactics {
			/**
			 * Chases after the ball as fast as possible.
			 */
			void chase(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags);

			/**
			 * If the player posses the ball,
			 * aims at an open angle at the enemy goal,
			 * and shoots the ball.
			 *
			 * If the player does not have the ball, chases after it.
			 *
			 * \param[in] force forces the player to shoot,
			 * even if the goal is completely blocked.
			 */
			void shoot(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags, const bool force = false);

			/**
			 * If the player posses the ball,
			 * aims at the target and shoots the ball.
			 *
			 * If the player does not have the ball, chases after it.
			 *
			 * \param[in] target the location to shoot the ball to.
			 */
			void shoot(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags, const Point target);

			/**
			 * I can't think of a good name for this function.
			 *
			 * Anyways, tries to move the ball as far away from friendly goal.
			 * If ball is not in possesion, chase after the ball.
			 * Otherwise, shoot the ball in the furthest possible direction.
			 * Useful for defenders.
			 *
			 * \param[in] flags movement flags for the robot
			 * most likely you want to disable avoid_friendly_defense.
			 */
			void repel(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const unsigned int flags);
		}
	}
}

#endif

