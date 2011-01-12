#ifndef AI_HL_STP_SKILL_SKILL_H
#define AI_HL_STP_SKILL_SKILL_H

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace SSM {
				// TODO: make skill not dependent on ssm
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

				class Skill;

				/**
				 * Defines the interface that skills can use.
				 */
				class Context {
					public:
						/**
						 * Sets the current skill to this,
						 * and executes this skill right after the current one finishes.
						 */
						virtual void execute_after(const Skill* skill) = 0;

						/**
						 * Indicates that this skill terminates successfully.
						 */
						virtual void finish() = 0;

						/**
						 * Something bad has happened.
						 */
						virtual void abort() = 0;

						/**
						 * Sets the current skill to this.
						 * This skill will be executed next tick.
						 */
						virtual void transition(const Skill* skill) = 0;
				};

				/**
				 * Skill is the most bottom layer in the STP paradigm.
				 *
				 * Think of FUNCTION POINTERS.
				 * Because a tactic "communicates" to a skill through parameters only,
				 * skills are made "stateless" immutable singletons.
				 *
				 * Note that skills are not completely "stateless",
				 * they are capable of writing stuff into the parameter class.
				 *
				 * Skills must NEVER call other skills directly,
				 * they must use the context to execute after or transition.
				 */
				class Skill {
					public:
						/**
						 * Executes the skill for this particular player.
						 *
						 * \param world the world associated.
						 *
						 * \param player the active player.
						 *
						 * \param ssm the skill state machine associated.
						 *
						 * \param param the skill parameters.
						 *
						 * \param context provides additional functionality.
						 */
						virtual void execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param, Context& context) const = 0;
				};
			}
		}
	}
}

#endif

