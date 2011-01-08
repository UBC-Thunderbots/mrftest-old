#ifndef AI_HL_STP_SSM_MOVE_BALL_H
#define AI_HL_STP_SSM_MOVE_BALL_H

#include "ai/hl/stp/ssm/ssm.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace SSM {
				/**
				 * Move the ball.
				 */
				class MoveBall : public SkillStateMachine {
					public:
						static const MoveBall* instance();

					private:
						const AI::HL::STP::Skill::Skill* initial() const;
				};
			}
		}
	}
}

#endif

