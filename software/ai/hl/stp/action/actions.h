#ifndef AI_HL_STP_ACTION_ACTIONS_H
#define AI_HL_STP_ACTION_ACTIONS_H

#include "ai/hl/stp/world.h"

/**
 * a couples of function to be implemented for stp to use
 * chase ( World, Player )
 * steal ( World, Player )
 * bump_ball ( World, Player )
 * drive_ball_to_target( World, Player )
 * spin_at_ball( World, Player, Orientation )
*/

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * TODO: implement steal ball
				 *
				 * Chases after the ball as fast as possible.
				 * Orient towards the ball.
				 */
				void chase(const World &world, Player::Ptr player);
				
				/**
				 * Chases after the ball as fast as possible. Orient towards Point target.
				 */
				void chase(const World &world, Player::Ptr player, Point target);

				/**
				 * Move the ball as far away as possible from friendly goal.
				 * Useful scenarios:
				 * - ball rolling towards the friendly goal
				 * - ball inside the defense area
				 */
				void repel(const World &world, Player::Ptr player);

				/**
				 * Convenient function.
				 * Move the players when NOT in any ACTIVE play.
				 * For example, during stop, preparing for kickoff, preparing for free kick.
				 */
				void free_move(const World &world, Player::Ptr player, const Point p);

				/**
				 * Blocks against a single enemy from shooting to our goal.
				 */
				void block(const World &world, Player::Ptr player, Robot::Ptr robot);
				
				/**
				 * Blocks against a single enemy from passing.
				 */
				void block_pass(const World &world, Player::Ptr player, Robot::Ptr robot);
			}
		}
	}
}

#endif

