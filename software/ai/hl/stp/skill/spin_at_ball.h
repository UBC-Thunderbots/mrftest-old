#ifndef AI_HL_STP_SKILL_SPIN_AT_BALL_H
#define AI_HL_STP_SKILL_SPIN_AT_BALL_H

#include "ai/hl/stp/skill/skill.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * Spin at the ball.
				 */
				class SpinAtBall : public Skill {
					public:
						const static SpinAtBall* instance();

						const Skill* execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const;
				};
			}
		}
	}
}

#endif

