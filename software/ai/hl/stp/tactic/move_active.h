#ifndef AI_HL_STP_TACTIC_MOVE_ACTIVE_H
#define AI_HL_STP_TACTIC_MOVE_ACTIVE_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Move to a location specified by dest.
				 *
				 * \param[in] dest the location to move to.
				 */
				Tactic::Ptr move_active(World world, const Point dest, const Angle orientation);
			}
		}
	}
}

#endif

