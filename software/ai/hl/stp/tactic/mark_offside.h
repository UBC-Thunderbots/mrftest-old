#ifndef AI_HL_STP_TACTIC_MARK_OFFSIDE_H
#define AI_HL_STP_TACTIC_MARK_OFFSIDE_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {

				/**
				 * Marks the offside player to defend against one time plays
				 *
				 * \input[in] target - the enemy player to target
				 */
				Tactic::Ptr mark_offside(World world);

			}
		}
	}
}

#endif

