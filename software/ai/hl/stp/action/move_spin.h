#ifndef AI_HL_STP_ACTION_MOVE_SPIN_H
#define AI_HL_STP_ACTION_MOVE_SPIN_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move to a particular location and stop.
				 * Spin while moving and continue spinning at the point.
				 */
				void move_spin(Player player, const Point dest);
			}
		}
	}
}

#endif

