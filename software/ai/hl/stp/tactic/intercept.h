#pragma once

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Intercept
				 * Active tactic.
				 * intercepts the ball
				 * target defaults to enemy goal
				 */
				Tactic::Ptr intercept(World world);
			}
		}
	}
}
