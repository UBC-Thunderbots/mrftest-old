#ifndef AI_HL_STP_EVALUATION_BALL_H
#define AI_HL_STP_EVALUATION_BALL_H

#include "ai/hl/stp/world.h"
#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Can pass.
				 */
				bool can_pass(const World& world, Player::CPtr passer, Player::CPtr passee);

				/**
				 * Can pass from p1 to p2?
				 */
				bool can_pass(const World& world, const Point p1, const Point p2);

				/**
				 * Ball is within pivot threshold.
				 */
				bool ball_in_pivot_thresh(const World &world, Player::CPtr player);

				/**
				 * The ball is right in front of the player.
				 */
				bool possess_ball(const World &world, Player::CPtr player);

				/**
				 * Ball is right in front of the robot.
				 */
				bool possess_ball(const World &world, Robot::Ptr robot);

				/**
				 * Finds a friendly player with the ball.
				 */
				Player::CPtr calc_friendly_baller(const World &world);

				/**
				 * Finds an enemy player with the ball.
				 */
				Robot::Ptr calc_enemy_baller(const World &world);

				/**
				 * Finds grab ball position
				 */
				Point grab_ball(const World &world, Player::Ptr player);

				/**
				 * Distance from the front to be considered ball possession.
				 */
				extern DoubleParam pivot_threshold;
			}
		}
	}
}

#endif

