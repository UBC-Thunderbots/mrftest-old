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
				bool can_pass(const World &world, Player::CPtr passer, Player::CPtr passee);

				/**
				 * Can pass from p1 to p2.
				 * Ignores position of friendly robots.
				 */
				bool can_pass(const World &world, const Point p1, const Point p2);

				/**
				 * Checks if a pass is possible for the pair of enemy
				 */
				bool enemy_can_pass(const World &world, const Robot::Ptr passer, const Robot::Ptr passee);

				/**
				 * Checks if passee is facing towards the ball so it can receive.
				 */
				bool passee_facing_ball(const World &world, Player::CPtr passee);

				/**
				 * Check if passee is facing towards passer so it can receive.
				 * Not sure why we need passer instead of ball.
				 */
				bool passee_facing_passer(Player::CPtr passer, Player::CPtr passee);

				/**
				 * Checks if a passee is suitable.
				 */
				bool passee_suitable(const World &world, Player::CPtr passee);

				/**
				 * Obtains a player who can be a passee.
				 * WARNING:
				 * This function has built-in hysterysis.
				 * Calls will return the previously chosen player if possible.
				 */
				Player::CPtr select_passee(const World &world);

				/**
				 * Checks if this direction is valid for shooting
				 * for indirect pass.
				 */
				bool can_shoot_ray(const World &world, Player::CPtr player, double orientation);

				/**
				 * Calculates the best shooting angle.
				 */
				std::pair<bool, double> best_shoot_ray(const World &world, const Player::CPtr player);

				Point calc_fastest_grab_ball_dest_if_baller_shoots(const World &world, const Point player_pos);


				extern IntParam ray_intervals;

				extern DoubleParam max_pass_ray_angle;

				extern DoubleParam ball_pass_velocity;
			}
		}
	}
}

#endif

