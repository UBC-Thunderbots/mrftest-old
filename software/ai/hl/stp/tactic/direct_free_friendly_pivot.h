#ifndef AI_HL_STP_TACTIC_DIRECT_FREE_FRIENDLY_PIVOT_H
#define AI_HL_STP_TACTIC_DIRECT_FREE_FRIENDLY_PIVOT_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {

				/**
				 * Approaches a target with autochip on. Used for direct free friendlies.
				 */
				Tactic::Ptr direct_free_friendly_pivot(World world);
			}
		}
	}
}

#endif
