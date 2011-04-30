#ifndef AI_HL_STP_ACTION_STOP_H
#define AI_HL_STP_ACTION_STOP_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Stop at the current location.
				 */
				void stop(const World& world, Player::Ptr player);
			}
		}
	}
}

#endif

