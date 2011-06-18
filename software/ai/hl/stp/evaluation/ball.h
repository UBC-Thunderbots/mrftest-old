#ifndef AI_HL_STP_EVALUATION_BALL_H
#define AI_HL_STP_EVALUATION_BALL_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Ball is within pivot threshold.
				 */
				bool ball_in_pivot_thresh(const World &world, Player::CPtr player);

				/**
				 * The ball is right in front of the player.
				 */
				bool possess_ball(const World &world, Player::CPtr player);

				/**
				 * Finds a friendly player with the ball.
				 */
				Player::CPtr calc_friendly_baller(const World &world);

				/**
				 * Finds an enemy player with the ball.
				 */
				Robot::Ptr calc_enemy_baller(const World &world);
			}
		}
	}
}

#endif

