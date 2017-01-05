#pragma once

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move Spin
				 *
				 * Not intended for goalie use
				 *
				 * Move to a particular location and stop. Spin while moving and
				 * continue spinning at the point.
				 */
				void move_spin(Player player, const Point dest);
			}
		}
	}
}