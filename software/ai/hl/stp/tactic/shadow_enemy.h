#ifndef AI_HL_STP_TACTIC_SHADOW_ENEMY_H
#define AI_HL_STP_TACTIC_SHADOW_ENEMY_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/enemy.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Follows the enemy, index is used to determine which closest enemy. 0 is closest, 1 is second closest, so forth.
				 */
				Tactic::Ptr shadow_enemy(World world, unsigned int index);
			}
		}
	}
}

#endif

