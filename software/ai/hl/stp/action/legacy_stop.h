#pragma once
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
