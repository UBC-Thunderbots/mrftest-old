#ifndef AI_HL_STP_SSM_GET_BALL_H
#define AI_HL_STP_SSM_GET_BALL_H

#include "ai/hl/stp/ssm/ssm.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace SSM {
				/**
				 * Finds a way to get the ball,
				 * either through chasing or stealing.
				 */
				const SkillStateMachine* get_ball();
			}
		}
	}
}

#endif

