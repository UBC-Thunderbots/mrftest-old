#ifndef AI_HL_STP_TACTIC_PASS_SIMPLE_H
#define AI_HL_STP_TACTIC_PASS_SIMPLE_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Just toss the ball to someone in front.
				 */
				Tactic::Ptr passer_simple(const World &world);

				/**
				 * Stay at position and wait for pass.
				 * number: first passee is 0, second is 1 etc
				 */
				Tactic::Ptr passee_simple(const World &world, unsigned number);

				/**
				 * Look at where the ball is heading,
				 * and catch it.
				 */
				Tactic::Ptr follow_baller(const World &world);
			}
		}
	}
}

#endif

