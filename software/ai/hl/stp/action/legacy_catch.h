#pragma once

#include "ai/hl/stp/action/action.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				bool legacy_catch_ball(World world, Player player, Point target);
				bool legacy_catch_stopped_ball(World world, Player player, Point target);
			}
		}
	}
}
