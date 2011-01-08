#ifndef AI_HL_STP_SKILL_SKILL_H
#define AI_HL_STP_SKILL_SKILL_H

#include "ai/hl/world.h"

#include <ctime>

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
						virtual Skill* execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, Param& param) const = 0;
				};

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
						virtual Skill* initial() const = 0;
				};
			}
		}
	}
}

#endif

