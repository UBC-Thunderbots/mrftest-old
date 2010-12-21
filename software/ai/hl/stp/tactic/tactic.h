#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/world.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <ctime>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * A tactic is a layer in the STP paradigm.
				 */
				class Tactic : public ByRef {
					public:
						typedef RefPtr<Tactic> Ptr;

						/**
						 * Constructor for tactic.
						 *
						 * \param [in] active indicates if this is an active tactic.
						 *
						 * \param [in] timeout the maximum time this tactic will execute.
						 */
						Tactic(AI::HL::W::World &world, bool active = false, double timeout = 5.0);

						~Tactic();

						/**
						 * Indicates if this tactic is done with its task.
						 * An inactive tactic will always be done.
						 */
						virtual bool done() const;

						/**
						 * Time since this tactic was created.
						 */
						double elapsed_time() const;

						/**
						 * Checks if the current tactic is an active tactic.
						 */
						bool active() const;

						/**
						 * Changes the player associated with this tactic.
						 */
						void set_player(AI::HL::W::Player::Ptr p);

						/**
						 * Scoring function to indicate how preferable this particular player is.
						 * There is constraint on the range of return values.
						 * The highest scoring player is simply chosen for the task.
						 */
						virtual double score(AI::HL::W::Player::Ptr player) const = 0;

						/**
						 * Drive some actual players.
						 */
						virtual void execute() = 0;

					protected:
						AI::HL::W::World &world;
						AI::HL::W::Player::Ptr player;

					private:
						bool active_;
						const double timeout_;
						std::time_t start_time;

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

