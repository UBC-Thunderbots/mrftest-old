#ifndef AI_HL_STP_EVALUATION_PASS_H
#define AI_HL_STP_EVALUATION_PASS_H

#include "ai/hl/stp/world.h"
#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Can pass?
				 */
				bool can_pass(const World& world, Player::CPtr passer, Player::CPtr passee);

				/**
				 * Can pass from p1 to p2.
				 * Ignores position of friendly robots.
				 */
				bool can_pass(const World& world, const Point p1, const Point p2);

				/**
				 * Checks if a pass is possible for the pair of enemy
				 */
				bool enemy_can_pass(const World &world, const Robot::Ptr passer, const Robot::Ptr passee);

				/**
				 * Checks if passee is facing towards the ball so it can receive.
				 */
				bool passee_facing_ball(const World& world, Player::CPtr passee);

				/**
				 * Check if passee is facing towards passer so it can receive.
				 * Not sure why we need passer instead of ball.
				 */
				bool passee_facing_passer(Player::CPtr passer, Player::CPtr passee);

				/**
				 * Checks if a passee is suitable.
				 */
				bool passee_suitable(const World& world, Player::CPtr passee);
				
				/**
				 * Obtains a random player who can be passee.
				 */
				Player::CPtr find_random_passee(const World& world);
			}
		}
	}
}

#endif

