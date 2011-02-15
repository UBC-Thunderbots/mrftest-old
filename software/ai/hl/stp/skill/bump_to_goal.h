#ifndef AI_HL_STP_SKILL_BUMP_TO_GOAL_H
#define AI_HL_STP_SKILL_BUMP_TO_GOAL_H

#include "ai/hl/stp/skill/skill.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * An implementation of the bump to goal skill in the STP paper.
				 * Attempts to bump the ball when the kicker is not ready.
				 *
				 * Terminates once it reaches the ball's position or if kicker is ready (3 sec)
				 */
				const Skill *bump_to_goal();
			}
		}
	}
}

#endif

