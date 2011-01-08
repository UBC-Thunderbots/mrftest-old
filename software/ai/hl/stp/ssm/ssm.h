#ifndef AI_HL_STP_SSM_SSM_H
#define AI_HL_STP_SSM_SSM_H

#include "ai/hl/stp/skill/skill.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace SSM {
				/**
				 * SSM in the Skill layer.
				 * SSM is a singleton, and does not contain any player or world info.
				 * Its purpose is just to set up the first state.
				 */
				class SkillStateMachine {
					public:
						/**
						 * Obtains the first skill.
						 */
						virtual const AI::HL::STP::Skill::Skill* initial() const = 0;
				};
			}
		}
	}
}

#endif

