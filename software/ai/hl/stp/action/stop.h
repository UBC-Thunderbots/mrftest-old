#ifndef AI_HL_STP_ACTION_STOP_H
#define AI_HL_STP_ACTION_STOP_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Stop
				 *
				 * Intended for goalie use
				 *
				 * Stop at the current location.
				 */
				void stop(World world, Player player);
			}
		}
	}
}

#endif

