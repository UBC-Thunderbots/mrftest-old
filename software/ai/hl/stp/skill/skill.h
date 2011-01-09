#ifndef AI_HL_STP_SKILL_SKILL_H
#define AI_HL_STP_SKILL_SKILL_H

#include "ai/hl/world.h"
#include <set>

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

					/**
					 * Move flags
					 */
					unsigned int move_flags;

					/**
					 * Move priority.
					 * Not always used.
					 * Active tactics ALWAYS have HIGH priority.
					 */
					unsigned int move_priority;

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
				 * Allows all skill to have a single clean interface,
				 * and gives more flexibility for skills.
				 * Also, allows transitions to be stored.
				 *
				 * TODO: find a clean way to use this.
				 */
				class Context {
					public:
						Context(AI::HL::W::World& w, AI::HL::W::Player::Ptr pl, Param& pr);

						/**
						 * The world associated.
						 */
						AI::HL::W::World& world();

						/**
						 * The player associated.
						 */
						AI::HL::W::Player::Ptr player();

						/**
						 * The skill state machine associated.
						 */
						const AI::HL::STP::SSM::SkillStateMachine* ssm() const;

						/**
						 * The parameters associated.
						 * Skills may modify this.
						 */
						Param& param();

						/**
						 * Sets the skill to be used in the next tick.
						 */
						void transition(const Skill* skill);

						/**
						 * Executes a different skill.
						 */
						void execute(const Skill* skill);

					protected:
						AI::HL::W::World& world_;
						AI::HL::W::Player::Ptr player_;
						Param& param_;

						const Skill* next_skill;

						/**
						 * We can check if skills ever loop to itself.
						 */
						std::set<const Skill*> history;
				};

				/**
				 * A skill is the bottom layer in the STP paradigm.
				 * A skill is a stateless immutable singleton.
				 * Its purpose is similar to that of a function pointer.
				 *
				 * Note that skills are not completely stateless,
				 * they are capable of writing stuff into the parameter class.
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

