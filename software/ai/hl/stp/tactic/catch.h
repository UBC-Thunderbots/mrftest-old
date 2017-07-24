#pragma once

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				Tactic::Ptr catch_ball(World world);
				Tactic::Ptr just_catch_ball(World world);
			}
		}
	}
}
