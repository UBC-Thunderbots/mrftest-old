#pragma once

#include "ai/hl/stp/action/action.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				void catch_stopped_ball(caller_t& ca, World world, Player player);
				void catch_ball(caller_t& ca, World world, Player player, Point target);
				void catch_ball_quick(caller_t& ca, World world, Player player);
			}
		}
	}
}
