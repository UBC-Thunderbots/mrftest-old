#ifndef AI_HL_STP_TACTIC_CHASE_H
#define AI_HL_STP_TACTIC_CHASE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Chase the ball.
				 */
				Tactic::Ptr chase(AI::HL::W::World &world);
			}
		}
	}
}

#endif

