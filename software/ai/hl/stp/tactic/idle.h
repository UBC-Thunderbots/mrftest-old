#ifndef AI_HL_STP_TACTIC_IDLE_H
#define AI_HL_STP_TACTIC_IDLE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Idle
				 * Not Active Tactic
				 * Stay at the same position and do nothing.
				 */
				Tactic::Ptr idle(World world);
			}
		}
	}
}

#endif

