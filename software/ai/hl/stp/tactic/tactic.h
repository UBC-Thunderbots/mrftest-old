#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/world.h"
#include "ai/hl/stp/skill/skill.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/ssm/ssm.h"
#include "util/byref.h"
#include "util/registerable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * A tactic is a layer in the STP paradigm.
				 * It runs every tick.
				 *
				 * A typical subclass shall derive execute(),
				 * and optionally player_changed().
				 *
				 * It may choose to use the Skill layer.
				 * If it does so, it must set a suitable Skill State Machine (SSM) every tick,
				 * as well as all the parameters.
				 *
				 * In other words, a tactic may only communicate with the skill layer through SSM and parameters.
				 *
				 * To prevent rapid fluctuation of parameters,
				 * hysteresis is recommended.
				 */
				class Tactic : public ByRef {
					public:
						typedef RefPtr<Tactic> Ptr;

						/**
						 * Constructor for tactic.
						 *
						 * \param [in] active indicates if this is an active tactic.
						 */
						Tactic(AI::HL::W::World &world, bool active = false);

						/**
						 * Destructor.
						 */
						virtual ~Tactic();

						/**
						 * Indicates if this tactic is done with its task.
						 * An inactive tactic will always be done.
						 */
						virtual bool done() const;

						/**
						 * Checks if the current tactic is an active tactic.
						 */
						bool active() const;

						/**
						 * Changes the player associated with this tactic.
						 * A subclass should not call this.
						 */
						void set_player(AI::HL::W::Player::Ptr p);

						/**
						 * Sets the movement flag associated.
						 * A subclass should not call this.
						 */
						void set_move_flags(unsigned int f);

						/**
						 * Scoring function to indicate how preferable this particular player is.
						 * There is constraint on the range of return values.
						 * The highest scoring player is simply chosen for the task.
						 */
						virtual double score(AI::HL::W::Player::Ptr player) const = 0;

						/**
						 * This function is called every tick,
						 * sets things up, and call execute().
						 */
						void tick();

					protected:
						AI::HL::W::World &world;
						AI::HL::W::Player::Ptr player;

						/**
						 * The set of parameters for all skills with this tactic.
						 */
						AI::HL::STP::Skill::Param param;

						/**
						 * Handles the skill state machine.
						 */
						AI::HL::STP::Skill::ContextImpl context;

						/**
						 * Sets the SSM associated with this tactic.
						 */
						void set_ssm(const AI::HL::STP::SSM::SkillStateMachine* s);

						/**
						 * A subclass must override this function.
						 */
						virtual void execute() = 0;

					private:
						bool active_;

						/**
						 * Right now this is only used for a certain callback function.
						 */
						const AI::HL::STP::SSM::SkillStateMachine* ssm;

						/**
						 * Called when the player associated with this tactic is changed.
						 *
						 * Since this function is always called,
						 * it is a good place to put intialization code.
						 */
						virtual void player_changed();
				};
			}
		}
	}
}

#endif

