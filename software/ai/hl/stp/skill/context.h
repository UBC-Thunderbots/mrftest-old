#ifndef AI_HL_STP_SKILL_CONTEXT_H
#define AI_HL_STP_SKILL_CONTEXT_H

#include "ai/hl/stp/skill/skill.h"
#include "ai/hl/stp/ssm/ssm.h"
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
				class Context {
					public:
						/**
						 * The constructor.
						 */
						Context(AI::HL::W::World& w, Param& p);

						/**
						 * The skill state machine associated.
						 */
						const AI::HL::STP::SSM::SkillStateMachine* ssm() const;

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
						 * Runs the context.
						 */
						void run();

						/**
						 * Sets the skill to be used in the next tick.
						 */
						void transition(const Skill* skill);

						/**
						 * Executes immediately a different skill.
						 */
						void execute(const Skill* skill);

					protected:
						AI::HL::W::World& world;
						AI::HL::W::Player::Ptr player;
						Param& param;

						const AI::HL::STP::SSM::SkillStateMachine* ssm_;
						const Skill* next_skill;

						/**
						 * We can check if skills ever loop to itself.
						 */
						std::set<const Skill*> history;
				};
			}
		}
	}
}

#endif

