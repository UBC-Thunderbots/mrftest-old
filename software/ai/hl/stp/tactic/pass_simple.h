#ifndef AI_HL_STP_TACTIC_PASS_SIMPLE_H
#define AI_HL_STP_TACTIC_PASS_SIMPLE_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Passer Simple
				 * Active Tactic
				 * Just toss the ball to someone in front.
				 */
				Tactic::Ptr passer_simple(World world);

				/**
				 * Passee Simple
				 * Not Active Tactic
				 * Stay at position and wait for pass.
				 * number: first passee is 0, second is 1 etc
				 */
				Tactic::Ptr passee_simple(World world, unsigned number);

				/**
				 * Follow Baller
				 * Not Active Tactic
				 * Look at where the ball is heading,
				 * and catch it.
				 */
				Tactic::Ptr follow_baller(World world);
			}
		}
	}
}

#endif

