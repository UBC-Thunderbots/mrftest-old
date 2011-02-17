#ifndef AI_HL_STP_TACTIC_OFFEND_H
#define AI_HL_STP_TACTIC_OFFEND_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Go to good offensive position to park robots.
				 */
				Tactic::Ptr offend(const AI::HL::W::World &world);
			}
		}
	}
}

#endif

