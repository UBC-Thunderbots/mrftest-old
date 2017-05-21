#pragma once

#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/action/action.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move Spin
				 *
				 * Move to a particular location and stop. Spin while moving and
				 * continue spinning at the point.
				 */
				void move_spin(caller_t&, Player player, Point dest, Angle speed);
			}
		}
	}
}
