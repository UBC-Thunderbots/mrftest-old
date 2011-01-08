#ifndef AI_HL_STP_SKILL_DRIVE_TO_GOAL_H
#define AI_HL_STP_SKILL_DRIVE_TO_GOAL_H

#include "ai/hl/stp/skill/skill.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * An implementation of the drive to goal skill in the STP paper.
				 */
				class DriveToGoal : public Skill {
					public:
						const static DriveToGoal* instance();

						const Skill* execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const;
				};
			}
		}
	}
}

#endif

