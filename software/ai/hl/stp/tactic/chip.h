#ifndef AI_HL_STP_TACTIC_CHIP_H
#define AI_HL_STP_TACTIC_CHIP_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				
				/**
				 * Chip Targer
				 * Active Tactic
				 * Chip at a specified target.
				 */
				Tactic::Ptr chip_target(World world, const Coordinate target);
			}
		}
	}
}

#endif

