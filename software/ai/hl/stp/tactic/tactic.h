#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/world.h"
#include "ai/hl/stp/skill/skill.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/ssm/ssm.h"
#include "util/byref.h"
#include "util/registerable.h"

#include <set>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * A tactic is a layer in the STP paradigm.
				 * See STP paper section 4.1.
				 *
				 * It runs every tick.
				 * A subclass shall derive select(), execute(),
				 * optionally initialize() and done().
				 *
				 * Important tactics that deal with the ball are called active tactics.
				 * Only one such tactic is active at any given time.
				 * Other tactics must wait for the active tactic to finish.
				 * An active tactic must override done().
				 *
				 * A tactic may choose to use the Skill layer.
				 * To do so,
				 * it must set a suitable Skill State Machine (SSM) every tick,
				 * and its parameters.
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
						~Tactic();

						/**
						 * An active tactic must override this,
						 * and provide a condition when this tactic is completed.
						 */
						virtual bool done() const;

						/**
						 * Checks if the current tactic is an active tactic.
						 */
						bool active() const {
							return active_;
						}

						/**
						 * Changes the player associated with this tactic.
						 * A subclass should not call this.
						 */
						void set_player(AI::HL::W::Player::Ptr p);

						/**
						 * Selects a player from the set.
						 * A subclass must implement this function.
						 *
						 * \param[in] players a set of players to choose from
						 *
						 * \return a player to be used by this tactic
						 */
						virtual AI::HL::W::Player::Ptr select(const std::set<AI::HL::W::Player::Ptr>& players) const = 0;

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
						 * Sets the SSM associated with this tactic.
						 */
						void set_ssm(const AI::HL::STP::SSM::SkillStateMachine* s);

						/**
						 * Called when the tactic is first used,
						 * or when the player associated with this tactic is changed.
						 */
						virtual void initialize();

						/**
						 * The main execution of this tactic.
						 * This function runs every tick.
						 * A subclass must implement this function.
						 */
						virtual void execute() = 0;

						/**
						 * Indicates if the ssm associated with this tactic finished.
						 */
						bool ssm_done() const;

					private:
						const bool active_;

						/**
						 * Handles the skill state machine.
						 */
						AI::HL::STP::Skill::ContextImpl context;

						/**
						 * The SSM associated.
						 */
						const AI::HL::STP::SSM::SkillStateMachine* ssm;
				};
			}
		}
	}
}

#endif

