#ifndef AI_HL_STP_TACTIC_INTERCEPT_H
#define AI_HL_STP_TACTIC_INTERCEPT_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * intercepts the ball
				 * Active tactic.
				 * target defaults to enemy goal
				 */
				Tactic::Ptr intercept(const World &world, const Point target=Point(3.025,0));
			}
		}
	}
}

#endif

