#ifndef AI_HL_STP_TACTIC_CHIP_H
#define AI_HL_STP_TACTIC_CHIP_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				
				/**
				 * Chip at a specified target.
				 */
				Tactic::Ptr chip_target(const World &world, const Coordinate target);
			}
		}
	}
}

#endif

