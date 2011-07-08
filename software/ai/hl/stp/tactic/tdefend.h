#ifndef AI_HL_STP_TACTIC_TDEFEND_H
#define AI_HL_STP_TACTIC_TDEFEND_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * A tactic for Terence goalie.
				 */
				Tactic::Ptr tgoalie(const World &world, const size_t defender_role);

				/**
				 * A tactic for Terence defender 1 (MUST EXIST).
				 */
				Tactic::Ptr tdefender1(const World &world);

				/**
				 * A tactic for Terence defender 2.
				 */
				Tactic::Ptr tdefender2(const World &world);

			}
		}
	}
}

#endif

