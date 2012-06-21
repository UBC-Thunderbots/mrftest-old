#ifndef AI_HL_STP_TACTIC_FREE_KICK_PASS_H
#define AI_HL_STP_TACTIC_FREE_KICK_PASS_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * For now, robot moves towards ball, rotates 30 degrees below, then 30 degrees above,
				 * then to shooting position and shoots to target.
				 */
				Tactic::Ptr free_kick_pass(const AI::HL::W::World &world, const Point target, bool chip = false, double speed  = AI::HL::STP::BALL_MAX_SPEED);
			}
		}
	}
}

#endif
