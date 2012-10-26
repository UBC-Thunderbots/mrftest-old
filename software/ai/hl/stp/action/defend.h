#ifndef AI_HL_STP_ACTION_DEFEND_H
#define AI_HL_STP_ACTION_DEFEND_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move the defender to this location.
				 */
				void defender_move(World world, Player player, Point dest);
			}
		}
	}
}

#endif

