#ifndef AI_HL_STP_TACTIC_INTERCEPT_H
#define AI_HL_STP_TACTIC_INTERCEPT_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Intercept
				 * Active tactic.
				 * intercepts the ball
				 * target defaults to enemy goal
				 */
				Tactic::Ptr intercept(World world, Point target);
				Tactic::Ptr intercept(World world);
			}
		}
	}
}

#endif

