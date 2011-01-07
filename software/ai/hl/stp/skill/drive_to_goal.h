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
						DriveToGoal(AI::HL::W::World& w, AI::HL::W::Player::Ptr p);

						void run();
				};

			}
		}
	}
}

#endif

