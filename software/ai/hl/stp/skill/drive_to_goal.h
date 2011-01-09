#ifndef AI_HL_STP_SKILL_DRIVE_TO_GOAL_H
#define AI_HL_STP_SKILL_DRIVE_TO_GOAL_H

#include "ai/hl/stp/skill/skill.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * An implementation of the drive to goal skill in the STP paper.
				 * Attempts to push the ball towards the desired direction to kick.
				 *
				 * Terminates if the ball is kicked. 
				 */
				const Skill* drive_to_goal();
			}
		}
	}
}

#endif

