#ifndef AI_HL_STP_TACTIC_RAM_H
#define AI_HL_STP_TACTIC_RAM_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Ram
				 * Active Tactic
				 * Ram to ball; A tactic that implements the Ram action which gets the robot to run into the ball. 
				 */
				Tactic::Ptr ram(AI::HL::W::World world);
			}
		}
	}
}

#endif

