#ifndef AI_HL_STP_TACTIC_BASIC_H
#define AI_HL_STP_TACTIC_BASIC_H

#include "ai/hl/stp/tactic/tactic.h"

/**
 * This file contains some basic stateless tactics
 */

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Defends against a specified enemy.
				 */
				Tactic::Ptr block(AI::HL::W::World &world, AI::HL::W::Robot::Ptr robot);

				/**
				 * Nothing LOL.
				 */
				Tactic::Ptr idle(AI::HL::W::World &world);
			}
		}
	}
}

#endif

