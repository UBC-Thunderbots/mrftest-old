#ifndef AI_HL_STP_SKILL_CONTEXT_H
#define AI_HL_STP_SKILL_CONTEXT_H

#include "ai/hl/stp/skill/skill.h"
#include <set>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Skill {
				/**
				 * Allows skill to have functionality such as transition.
				 *
				 * Also, allows transitions to be stored.
				 *
				 * TODO:
				 * A context should only tie to one particular player.
				 */
				class ContextImpl : private Context {
					public:
						/**
						 * The constructor.
						 */
						ContextImpl(AI::HL::W::World& w, Param& p);

						/**
						 * TODO: fix this.
						 */
						void set_player(AI::HL::W::Player::Ptr p);

						/**
						 * Changes the current ssm associated.
						 * If the ssm stays the same, the skill will not be reset.
						 */
						void set_ssm(const AI::HL::STP::SSM::SkillStateMachine* ssm);

						/**
						 * The skill changes to that to of the start of the ssm.
						 */
						void reset_ssm();

						/**
						 * Checks if the context has reached a terminal state.
						 */
						bool done() const;

						/**
						 * Runs the context.
						 */
						void run();

					private:
						AI::HL::W::World& world;
						AI::HL::W::Player::Ptr player;
						Param& param;

						const AI::HL::STP::SSM::SkillStateMachine* ssm;

						const Skill* next_skill;
						bool execute_next_skill;

						/**
						 * We can check if skills ever loop to itself.
						 */
						std::set<const Skill*> history;

						/**
						 * Transition to a different skill.
						 * This skill is executed after the current skill finishes execute().
						 */
						void execute_after(const Skill* skill);

						/**
						 * Indicates that this skill terminates.
						 */
						void finish();

						/**
						 * Something bad has happened.
						 */
						void abort();

						/**
						 * Does not execute this skill immediately.
						 * But sets it for next tick.
						 */
						void transition(const Skill* skill);
				};
			}
		}
	}
}

#endif

