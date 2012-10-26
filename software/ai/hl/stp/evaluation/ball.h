#ifndef AI_HL_STP_EVALUATION_BALL_H
#define AI_HL_STP_EVALUATION_BALL_H

#include "ai/hl/stp/world.h"
#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				void tick_ball(World world);

				/**
				 * Finds a friendly player with the ball.
				 * Wrapper to stp's baller.
				 */
				Player calc_friendly_baller();

				/**
				 * Finds an enemy player with the ball.
				 */
				Robot calc_enemy_baller(World world);

				/**
				 * Ball is within pivot threshold.
				 */
				bool ball_in_pivot_thresh(World world, Player player);

				/**
				 * The ball is right in front of the player.
				 */
				bool possess_ball(World world, Player player);

				/**
				 * Ball is right in front of the robot.
				 */
				bool possess_ball(World world, Robot robot);

				/**
				 * Computes the best location to grab the ball,
				 * minimizing the time required.
				 * This is a wrapper to the function in ai/util.h
				 */
				Point calc_fastest_grab_ball_dest(World world, Player player);

				std::vector<Robot> enemies_by_grab_ball_dist();

				/**
				 * Distance from the front to be considered ball possession.
				 */
				extern DoubleParam pivot_threshold;
			}
		}
	}
}

#endif

