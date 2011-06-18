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
				 * If the player does not have the ball, chases after it.
				 *
				 * \return true if the robot shoots.
				 */
				bool shoot_goal(const World &world, Player::Ptr player);

				/**
				 * Shoots the ball to a target point with a double param kicking speed for passing
				 *
				 * \param[in] pass true if the player is to pass.					
				 *
				 * \return true if the robot shoots.
				 */	
				bool shoot_target(const World &world, Player::Ptr player, const Point target, bool pass);

				/**
				 * Determines whether or not the robot is facing within threshold degrees of the specified target
				 *
				 */
				bool within_angle_thresh(Player::Ptr player, const Point target, double threshold);

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
				bool shoot(const World &world, Player::Ptr player, const Point target, double tol = AI::HL::Util::shoot_accuracy, double delta = 1e9);

				/**
				 * Testing function designed for internal use & use with shoot_distance_test!!!
				 * \param[in] distance in m to shoot the ball.
				 * \param[in] a special ttesting param
				 * \return double the robot kick speed.
				 */
				double shoot_speed(double distance, double delta = 1e9, double alph=-1);

				/**
				 * Arm the kicker so that it kicks ball exact speed to stop at target (i.e. t= inf)
				 * or alternatively reach target at time delta from the current time ( may be moving )
				 * \return true if the constraints are achievable
				 */
				bool arm(const World &world, Player::Ptr player, const Point target, double delta = 1e10);

			}
		}
	}
}

#endif

