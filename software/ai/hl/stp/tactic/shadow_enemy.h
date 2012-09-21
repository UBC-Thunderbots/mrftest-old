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
				 * Follows the enemy
				 */
				Tactic::Ptr shadow_enemy(World world);
			}
		}
	}
}

#endif

