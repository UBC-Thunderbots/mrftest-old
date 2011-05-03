#ifndef AI_HL_STP_TACTIC_BLOCK_H
#define AI_HL_STP_TACTIC_BLOCK_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/enemy.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Blocks against an enemy from view of our goal.
				 */
				Tactic::Ptr block(const World &world, Enemy::Ptr enemy);
				
				/**
				 * Blocks against an enemy from passing.
				 */
				Tactic::Ptr block_pass(const World &world, Enemy::Ptr enemy);
			}
		}
	}
}

#endif

