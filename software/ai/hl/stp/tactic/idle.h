#ifndef AI_HL_STP_TACTIC_IDLE_H
#define AI_HL_STP_TACTIC_IDLE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Stay at the same position and do nothing.
				 */
				Tactic::Ptr idle(const AI::HL::W::World &world);
			}
		}
	}
}

#endif

