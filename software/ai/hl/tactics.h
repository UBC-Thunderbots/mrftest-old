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
			 * Chases after the ball as fast as possible.
			 */
			void chase(AI::HL::W::World& world, AI::HL::W::Player::Ptr player);

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
			void shoot(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const bool force = false);

			/**
			 * If the player posses the ball,
			 * aims at the target and shoots the ball.
			 *
			 * If the player does not have the ball, chases after it.
			 *
			 * \param[in] force forces the player to shoot,
			 * even if the goal is completely blocked.
			 */
			void shoot(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const Point target);

			/**
			 * I can't think of a good name for this function.
			 *
			 * Anyways, tries to move the ball as far away from friendly goal.
			 * If ball is not in possesion, chase after the ball.
			 * Otherwise, shoot the ball in the furthest possible direction.
			 * Useful for defenders.
			 *
			 * \param[in] avoid_friendly_defense disallows the player
			 * to enter friendly defense area.
			 * If you use this function, it looks like an emergency
			 * so most likely you want the defender to do this.
			 */
			void repel(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const bool avoid_friendly_defense = false);
		}
	}
}

#endif

