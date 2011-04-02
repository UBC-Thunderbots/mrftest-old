#ifndef AI_HL_STP_ACTION_ACTIONS_H
#define AI_HL_STP_ACTION_ACTIONS_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Chases after the ball as fast as possible. Orient towards the ball.
				 */
				void chase(const World &world, Player::Ptr player, const unsigned int flags = 0);
				
				/**
				 * Chases after the ball as fast as possible. Orient towards Point target.
				 */
				void chase(const World &world, Player::Ptr player, Point target, const unsigned int flags = 0);

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
				 * Blocks against a single enemy from shooting to our goal.
				 */
				void block(const World &world, Player::Ptr player, const unsigned int flags, Robot::Ptr robot);
				
				/**
				 * Blocks against a single enemy from passing.
				 */
				void block_pass(const World &world, Player::Ptr player, const unsigned int flags, Robot::Ptr robot);
			}
		}
	}
}

#endif

