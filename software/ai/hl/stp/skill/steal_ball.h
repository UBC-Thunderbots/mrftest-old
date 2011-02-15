#ifndef AI_HL_STP_SKILL_STEAL_BALL_H
#define AI_HL_STP_SKILL_STEAL_BALL_H

#include "ai/hl/stp/skill/skill.h"
#include "uicomponents/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * Steal the ball!
				 * Useful if an enemy robot has the ball.
				 *
				 * If there is no enemy nearby, changes to go_to_ball.
				 */
				const Skill *steal_ball();
			}
		}
	}
}

#endif

