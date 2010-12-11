#ifndef AI_HL_STP_TACTIC_BASIC_H
#define AI_HL_STP_TACTIC_BASIC_H

#include "ai/hl/stp/tactic/tactic.h"

/**
 * This file contains some basic stateless tactics
 */

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Goto some place.
			 */
			Tactic::Ptr move(AI::HL::W::World &world, const Point dest);

			/**
			 * A standard lone goalie tactic.
			 */
			Tactic::Ptr defend_goal(AI::HL::W::World &world);

			/**
			 * Move the ball away from our own goal at all cost.
			 */
			Tactic::Ptr repel(AI::HL::W::World &world);

			/**
			 * Defends against a specified enemy.
			 */
			Tactic::Ptr block(AI::HL::W::World &world, AI::HL::W::Robot::Ptr robot);

			/**
			 * Shoot for the enemy goal.
			 */
			Tactic::Ptr shoot(AI::HL::W::World &world);

			/**
			 * Shoot a specified target.
			 */
			Tactic::Ptr shoot(AI::HL::W::World &world, const Point target);

			/**
			 * Go for the ball.
			 */
			Tactic::Ptr chase(AI::HL::W::World &world);

			/**
			 * Nothing LOL.
			 */
			Tactic::Ptr idle(AI::HL::W::World &world);
		}
	}
}


#endif

