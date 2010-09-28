#ifndef AI_HL_STRATEGY_H
#define AI_HL_STRATEGY_H

#include "ai/hl/world.h"

/**
 * Tactics are helper methods/classes which allow strategies
 * to compose commonly used complex movement.
 *
 * It also serves to unify definition.
 */
namespace AI {
	namespace HL {
		namespace Tactics {

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
			void shoot(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const bool force = false);

			/**
			 * If the player posses the ball,
			 * aims at the target and shoots the ball.
			 *
			 * If the player does not have the ball, chases after it.
			 *
			 * \param[in] force forces the player to shoot,
			 * even if the goal is completely blocked.
			 */
			void shoot(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, Point target);

		}
	}
}

#endif

