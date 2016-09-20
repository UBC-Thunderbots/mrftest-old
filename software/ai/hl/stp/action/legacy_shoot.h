#pragma once
#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Shoot Goal
				 *
				 * Not intended for goalie use
				 *
				 * Shoots the ball at the largest open angle of the enemy goal.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_goal(World world, Player player, bool use_reduced_radius = true);

				/**
				 * Shoot Target
				 *
				 * Not intended for goalie use
				 *
				 * WARNING: should use shoot_region or shoot_pass
				 * Shoots the ball to a target point with a double param kicking speed for passing
				 *
				 * \param[in] pass true if the player is to pass.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_target(World world, Player player, const Point target,
					double velocity = BALL_MAX_SPEED, bool chip = false);

				/**
				 * Pivot Shoot
				 *
				 * Not intended for goalie use
				 *
				 * Pivots around the ball until within angle threshold, then shoots the ball
				 */
				void pivot_shoot(World world, Player player, const Point target,
					double velocity = BALL_MAX_SPEED);

				/**
				 * Shoot Pass
				 *
				 * Not intended for goalie use
				 *
				 * Directly shoots to a player.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_pass(World world, Player shooter, const Point target);

				/**
				 * Shoot Pass
				 *
				 * Not intended for goalie use
				 *
				 * Directly shoots to a player.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_pass(World world, Player shooter, const Point target,
					Angle angle_tol);

				/**
				 * Shoot Pass
				 *
				 * Not intended for goalie use
				 *
				 * Shoot to a player.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_pass(World world, Player shooter, Player target);

				/**
				 * Shoot Speed
				 *
				 * Not intended for goalie use
				 *
				 * Testing function designed for internal use & use with
				 * shoot_distance_test!!!
				 *
				 * \param[in] distance in m to shoot the ball.
				 * \param[in] a special ttesting param
				 * \return double the robot kick speed.
				 */
				double shoot_speed(double distance, double delta = 1e9,
					double alph = -1);
			}
		}
	}
}
