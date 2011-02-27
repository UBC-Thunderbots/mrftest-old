#ifndef AI_HL_STP_ACTION_SHOOT_H
#define AI_HL_STP_ACTION_SHOOT_H

#include "ai/hl/stp/world.h"
#include "geom/rect.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
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
				void shoot(const World &world, Player::Ptr player, const unsigned int flags = 0, const bool force = false);

				/**
				 * If the player posses the ball,
				 * aims at the target and shoots the ball.
				 *
				 * If the player does not have the ball, chases after it.
				 *
				 * \param[in] target the location to shoot the ball to.
				 */
				void shoot(const World &world, Player::Ptr player, const Point target, const unsigned int flags = 0, const bool force = false);

				/**
				 * If the player posses the ball,
				 * aims at the region and shoots the ball.
				 *
				 * If the player does not have the ball, chases after it.
				 *
				 * \param[in] region the location to shoot the ball to.
				 */
				void shoot(const World &world, Player::Ptr player, const Rect region, const unsigned int flags = 0, const bool force = false);
			}
		}
	}
}

#endif

