#ifndef AI_HL_STP_TACTIC_PASS_H
#define AI_HL_STP_TACTIC_PASS_H

#include "ai/hl/tactic/tactic.h"
#include "ai/hl/evaluation/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Moves a passer to this location, getting ready to shoot.
				 * This is an active tactic.
				 */
				Tactic::Ptr passer_position(AI::HL::W::World& world, Point p);

				/**
				 * Moves a passee to this location, getting ready to catch the ball.
				 * This is an active tactic.
				 */
				Tactic::Ptr passee_position(AI::HL::W::World& world, Point p);
			}
		}
	}
}

#endif

