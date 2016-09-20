#pragma once

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Defender Move
				 *
				 * Not intended for goalie use
				 *
				 * Move the defender to this location.
				 *
				 * active_baller: whether there is an active baller
				 */
				void defender_move(World world, Player player, Point dest, bool active_baller);
			}
		}
	}
}
