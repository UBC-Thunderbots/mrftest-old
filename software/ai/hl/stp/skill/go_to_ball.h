#ifndef AI_HL_STP_SKILL_GO_TO_BALL_H
#define AI_HL_STP_SKILL_GO_TO_BALL_H

#include "ai/hl/stp/skill/skill.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * An implementation of the go to ball skill in the STP paper.
				 */
				class GoToBall : public Skill {
					public:
						const static GoToBall* instance();

						const Skill* execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const;
				};		
			}
		}
	}
}

#endif

