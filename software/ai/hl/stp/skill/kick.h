#ifndef AI_HL_STP_SKILL_KICK_H
#define AI_HL_STP_SKILL_KICK_H

#include "ai/hl/stp/skill/skill.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * A skill that kicks the ball if possible.
				 *
				 * This skill is a TERMINATING.
				 * Meaning that if the task is completed,
				 * will return a terminal skill.
				 */
				extern const Skill* kick();
			}
		}
	}
}

#endif

