#ifndef AI_HL_STP_TACTIC_CHASE_H
#define AI_HL_STP_TACTIC_CHASE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Finds a way to get the ball,
				 * either through chasing or stealing.
				 */
				Tactic::Ptr chase(AI::HL::W::World &world);
			}
		}
	}
}

#endif

