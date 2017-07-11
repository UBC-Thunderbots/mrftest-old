#pragma once

#include "ai/hl/stp/action/action.h"
#include "util/dprint.h"

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
				 */
				void shoot_goal(caller_t& ca, World world, Player player, bool chip = false, bool should_wait = true);

				/**
				 * Shoot Target
				 *
				 * Not intended for goalie use
				 *
				 * Shoots the ball to a target point with a double param kicking speed for passing
				 */
				void shoot_target(caller_t& ca, World world, Player player, const Point target, double velocity = BALL_MAX_SPEED, bool chip = false, bool should_wait = true);

				/**
				 * Shoot Target
				 *
				 * Not intended for goalie use
				 *
				 * Shoots the ball to a target point with a double param kicking speed for passing
				 */
				void catch_and_shoot_target(caller_t& ca, World world, Player player, const Point target, double velocity = BALL_MAX_SPEED, bool chip = false, bool should_wait = true);
				void catch_and_shoot_goal(caller_t& ca, World world, Player player, bool chip = false, bool should_wait = true);

                inline void wait_shoot(caller_t& ca, Player player) {
					while(!player.autokick_fired()) {
                        LOG_INFO(u8"not kicked");
						Action::yield(ca);
					}
                    LOG_INFO(u8"kicked");
				}
			}
		}
	}
}
