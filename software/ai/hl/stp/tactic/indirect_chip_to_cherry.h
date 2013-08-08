#ifndef AI_HL_STP_TACTIC_INDIRECT_CHIP_TO_CHERRY_H
#define AI_HL_STP_TACTIC_INDIRECT_CHIP_TO_CHERRY_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {

				/**
				 * Chip ball to a cherry picker.
				 * This is a stopgap hack for some things in RoboCup. Need to refactor this code soon.
				 */
				Tactic::Ptr indirect_chip_to_cherry(World world);
			}
		}
	}
}

#endif

