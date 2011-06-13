#ifndef AI_HL_STP_ACTION_SHOOT_H
#define AI_HL_STP_ACTION_SHOOT_H

#include "ai/hl/stp/world.h"
#include "geom/rect.h"
#include "util/param.h"
#include "ai/hl/util.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * If the player posses the ball,
				 * aims at an open angle at the enemy goal,
				 * and shoots the ball.
				 *
				 * WARNING:
				 * ONLY ONE ROBOT should be calling this action at any one time.
				 *
				 * If the player does not have the ball,
				 * chases after it.
				 *
				 * \return true if the robot shoots.
				 */
				bool shoot(const World &world, Player::Ptr player);
				
				bool shoot(const World &world, Player::Ptr player, const Point target);

				/**
				 * If the player posses the ball,
				 * aims at the region centred at target with radius tol and shoots the ball.
				 *
				 * If the player does not have the ball, chases after it.
				 *
				 * \param[in] region the location to shoot the ball to.
				 *
				 * \return true if the robot shoots.
				 */
				//bool shoot(const World &world, Player::Ptr player, const Point target, double tol = AI::HL::Util::shoot_accuracy, double delta = 1e9);

				/**
				 * Arm the kicker so that it kicks ball exact speed to stop at target (i.e. t= inf)
				 * or alternatively reach target at time delta from the current time ( may be moving )
				 * \return true if the constraints are achievable
				 */
				//bool arm(const World &world, Player::Ptr player, const Point target, double delta = 1e10);

				
			}
		}
	}
}

#endif

