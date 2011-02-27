#ifndef AI_HL_STP_ACTION_ACTIONS_H
#define AI_HL_STP_ACTION_ACTIONS_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Actions {
				/**
				 * Chases after the ball as fast as possible.
				 */
				void chase(const World &world, Player::Ptr player, const unsigned int flags);

				/**
				 * If the player posses the ball,
				 * aims at an open angle at the enemy goal,
				 * and shoots the ball.
				 *
				 * If the player does not have the ball, chases after it.
				 *
				 * \param[in] force forces the player to shoot,
				 * even if the goal is completely blocked.
				 */
				void shoot(const World &world, Player::Ptr player, const unsigned int flags, const bool force = false);

				/**
				 * If the player posses the ball,
				 * aims at the target and shoots the ball.
				 *
				 * If the player does not have the ball, chases after it.
				 *
				 * \param[in] target the location to shoot the ball to.
				 */
				void shoot(const World &world, Player::Ptr player, const unsigned int flags, const Point target, const double kick_power = 10.0);

				/**
				 * I can't think of a good name for this function.
				 *
				 * Anyways, tries to move the ball as far away from friendly goal.
				 * If ball is not in possesion, chase after the ball.
				 * Otherwise, shoot the ball in the furthest possible direction.
				 * Useful for defenders.
				 *
				 * \param[in] flags movement flags for the robot (most likely you want to disable AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE).
				 */
				void repel(const World &world, Player::Ptr player, const unsigned int flags);

				/**
				 * Convenient function.
				 * Move the players when NOT in any ACTIVE play.
				 * For example, during stop, preparing for kickoff, preparing for free kick.
				 */
				void free_move(const World &world, Player::Ptr player, const Point p);

				/**
				 * A single goalie and NO ONE ELSE defending the field.
				 */
				void lone_goalie(const World &world, Player::Ptr player);

				/**
				 * Blocks against a single enemy from shooting.
				 */
				void block(World &world, Player::Ptr player, const unsigned int flags, Robot::Ptr robot);
			}
		}
	}
}

#endif

