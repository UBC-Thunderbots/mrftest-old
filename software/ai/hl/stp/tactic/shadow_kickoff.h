#ifndef AI_HL_STP_TACTIC_SHADOW_KICKOFF_H
#define AI_HL_STP_TACTIC_SHADOW_KICKOFF_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/enemy.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Shadow a specific enemy robot on the enemy kickoff.
				 */
				Tactic::Ptr shadow_kickoff(const World &world, Enemy::Ptr enemy, const Coordinate default_loc);
				
				/**
				 * Shadow the ball (in freekicks).
				 */
				Tactic::Ptr shadow_ball(const World &world);
			}
		}
	}
}

#endif
