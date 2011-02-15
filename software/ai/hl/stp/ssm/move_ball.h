#ifndef AI_HL_STP_SSM_MOVE_BALL_H
#define AI_HL_STP_SSM_MOVE_BALL_H

#include "ai/hl/stp/ssm/ssm.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace SSM {
				/**
				 * Move the ball.
				 * Basically position the ball for a good position,
				 * and shoot the enemy goal if we can.
				 */
				const SkillStateMachine *move_ball();
			}
		}
	}
}

#endif

