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
					Param();
					bool can_kick;
					unsigned int move_flags;
					unsigned int move_type;
					unsigned int move_priority;
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
						 * Note that one can make use of tail recursion to execute and return a different skill.
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

