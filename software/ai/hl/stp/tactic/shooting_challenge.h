#ifndef AI_HL_STP_TACTIC_SHOOTING_CHALLENGE_H
#define AI_HL_STP_TACTIC_SHOOTING_CHALLENGE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Shooting Challenge
				 * Active Tactic
				 * Waits in position until the angle to the goal is clear of enemies. If it's clear of enemies, it shoots.
				 */
				Tactic::Ptr shooting_challenge(AI::HL::W::World world, double speed_ratio = 1.0);
			}
		}
	}
}

#endif
