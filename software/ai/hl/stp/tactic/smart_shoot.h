#ifndef AI_HL_STP_TACTIC_SMART_SHOOT_H
#define AI_HL_STP_TACTIC_SMART_SHOOT_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/*
				 * Smart Shoot
				 * Active Tactic
				 * The play checks to see if it is clear to shoot or not.
				 * If the path to the destination is clear, it shoots. If there is an obstacle, it will chip if the ball will clear.
				 * Otherwise, it will attempt to move back until it can clear.
				 */
				Tactic::Ptr smart_shoot(AI::HL::W::World world, const Point target, double speed_ratio = 1.0);
			}
		}
	}
}

#endif
