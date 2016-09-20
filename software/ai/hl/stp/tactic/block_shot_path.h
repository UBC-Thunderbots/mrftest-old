#pragma once

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/enemy.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * BlockShotPath
				 * Not Active Tactic
				 * Follows the enemy, index is used to determine which closest enemy. 0 is closest, 1 is second closest, so forth.
				 */
			  Tactic::Ptr block_shot_path(World world, unsigned int index, double max_dist = 4.0);
			}
		}
	}
}

