#ifndef AI_HL_STP_ACTION_PIVOT_H
#define AI_HL_STP_ACTION_PIVOT_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * SpinAtBall in STP paper.
				 *
				 * If the player does not have the ball, chase it.
				 * Rotates the player toward the target while holding the ball.
				 */
				void pivot(const World &world, Player::Ptr player, const Point target);

				/**
				 * Go chase after the ball and pivot towards the direction of target
				 */
				void chase_pivot(const World &world, Player::Ptr player, const Point target);
			}
		}
	}
}

#endif

