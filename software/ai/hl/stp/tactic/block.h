#pragma once

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/enemy.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Block Goal
				 * Non Active tactic
				 * Blocks against an enemy from view of our goal.
				 * Uses the block goal action
				 */
				Tactic::Ptr block_goal(World world, Enemy::Ptr enemy);

				/**
				 * Block Ball
				 * Non Active Tactic
				 * Blocks against an enemy from the ball / passing.
				 * Uses the block ball action
				 */
				Tactic::Ptr block_ball(World world, Enemy::Ptr enemy);
			}
		}
	}
}
