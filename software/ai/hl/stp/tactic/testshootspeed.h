#ifndef AI_HL_STP_TACTIC_TESTSHOOTSPEED_H
#define AI_HL_STP_TACTIC_TESTSHOOTSPEED_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/*
				* Test Shoot Speed
				* Active Tactic
				* A tactic implemented to test the ball shooting speed
				*/
				Tactic::Ptr test_shoot_speed(World world, bool force = false);

			}
		}
	}
}

#endif

