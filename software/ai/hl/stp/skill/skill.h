#ifndef AI_HL_STP_SKILL_SKILL_H
#define AI_HL_STP_SKILL_SKILL_H

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace SSM {
				class SkillStateMachine;
			}
		}
	}
}

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {

				/**
				 * Parameters that will be shared across skills go here.
				 */
				struct Param {
					bool can_kick;
				};

				/**
				 * A skill is the bottom layer in the STP paradigm.
				 * A skill is a stateless function object.
				 */
				class Skill {
					public:
						/**
						 * Executes the skill for this particular player.
						 *
						 * \return the skill it should transition to.
						 */
						virtual const Skill* execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const = 0;
				};
			}
		}
	}
}

#endif

