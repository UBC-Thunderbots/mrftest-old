#ifndef AI_HL_STP_ACTION_SHOOT_H
#define AI_HL_STP_ACTION_SHOOT_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {

				/**
				 * Shoots the ball at the largest open angle of the enemy goal.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_goal(World world, Player player, bool use_reduced_radius = true);

				/**
				 * WARNING: should use shoot_region or shoot_pass
				 * Shoots the ball to a target point with a double param kicking speed for passing
				 *
				 * \param[in] pass true if the player is to pass.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_target(World world, Player player, const Point target, double velocity = BALL_MAX_SPEED);

				/**
				 * Directly shoots to a player.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_pass(World world, Player shooter, const Point target);

				/**
				 * Directly shoots to a player.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_pass(World world, Player shooter, const Point target, Angle angle_tol);


				/**
				 * Shoot to a player.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool shoot_pass(World world, Player shooter, Player target);

				/**
				 * Testing function designed for internal use & use with shoot_distance_test!!!
				 * \param[in] distance in m to shoot the ball.
				 * \param[in] a special ttesting param
				 * \return double the robot kick speed.
				 */
				double shoot_speed(double distance, double delta = 1e9, double alph = -1);
			}
		}
	}
}

#endif

