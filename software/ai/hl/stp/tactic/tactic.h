#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/world.h"
#include "ai/hl/stp/skill/skill.h"
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
				 * It may choose to use the Skill layer.
				 * If it does so, it has to set the appropriate skill state machine,
				 * as well as all the parameters.
				 *
				 * To prevent rapid fluctuation of parameters,
				 * a hysteresis is recommended.
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
						 */
						void set_player(AI::HL::W::Player::Ptr p);

						/**
						 * Sets the movement flag associated.
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
						virtual void tick();

					protected:
						AI::HL::W::World &world;
						AI::HL::W::Player::Ptr player;

						/**
						 * Movement flags associated.
						 */
						unsigned int move_flags;

						/**
						 * The set of parameters associated with this tactic.
						 */
						AI::HL::STP::Skill::Param param;

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
						 * The ssm associated.
						 */
						const AI::HL::STP::SSM::SkillStateMachine* ssm;

						/**
						 * The current skill.
						 */
						const AI::HL::STP::Skill::Skill* skill;

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

