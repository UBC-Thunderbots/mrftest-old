#pragma once

#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Test new move action:
                 * moves to dest then move back to its original position
				 */
				Tactic::Ptr move_test(World world, Point dest);

				Tactic::Ptr move_test_orientation(World world, Point dest);

				Tactic::Ptr shoot_test(World world);

				Tactic::Ptr catch_test(World world);
			}
		}
	}
}

