#ifndef AI_HL_STP_SKILL_SKILL_H
#define AI_HL_STP_SKILL_SKILL_H

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * Parameters that will be shared across skills go here.
				 */
				struct Param {
					Param();

					/**
					 * Move flags.
					 */
					unsigned int move_flags;

					/**
					 * The default move priority.
					 * Note that ACTIVE tactics and certain skills ignore this and use HIGH priority.
					 */
					AI::Flags::MovePrio move_priority;

					/**
					 * A desired distance for SpinAtBall.
					 */
					double pivot_distance;

					/**
					 * A desired orientation for SpinAtBall.
					 */
					double pivot_orientation;
				};

				class Context;

				/**
				 * Skill is the most bottom layer in the STP paradigm.
				 *
				 * Think of FUNCTION POINTERS.
				 * Because a tactic "communicates" to a skill through parameters only,
				 * skills are made "stateless" immutable singletons.
				 *
				 * Note that skills are not completely "stateless",
				 * they are capable of writing stuff into the parameter class.
				 */
				class Skill {
					public:
						/**
						 * Executes the skill for this particular player.
						 *
						 * \param[in] world the world associated.
						 *
						 * \param[in] player the active player.
						 *
						 * \param[in] param the skill parameters.
						 *
						 * \param[in,out] context allows additional functionality such as state transitions.
						 */
						virtual void execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, Param& param, Context& context) const = 0;
				};

				/**
				 * A terminal state always transition to itself.
				 * Denotes that the SSM completes succesfully.
				 */
				extern const Skill* finish();

				/**
				 * A terminal state always transition to itself.
				 * Denotes that the SSM failed.
				 */
				extern const Skill* fail();
			}
		}
	}
}

#endif

