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
				 */
				Tactic::Ptr intercept(const World &world);
			}
		}
	}
}

#endif

