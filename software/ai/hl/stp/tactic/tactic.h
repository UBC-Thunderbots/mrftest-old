#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/world.h"
#include "util/byref.h"
#include "util/registerable.h"

#include <ctime>

namespace AI {
	namespace HL {
		namespace STP {

			/**
			 * A tactic is a layer in the STP paradigm.
			 */
			class Tactic : public ByRef {
				public:
					typedef RefPtr<Tactic> Ptr;

					/**
					 * Constructor for tactic.
					 *
					 * \param [in] timeout the maximum time this tactic will execute.
					 */
					Tactic(AI::HL::W::World& world, double timeout = 5.0);

					~Tactic();

					/**
					 * Indicates if this tactic is done with its task.
					 * An inactive tactic will always be done.
					 */
					virtual bool done() const;

					/**
					 * Checks if the tactic took more time than it is supposed to.
					 */
					bool timed_out() const;

					/**
					 * Changes the player associated with this tactic.
					 */
					void set_player(AI::HL::W::Player::Ptr p);

					/**
					 * Scoring function
					 * to indicate how preferable this particular player is.
					 *
					 * \return between 1 and 0, indicating the preference.
					 */
					virtual double score(AI::HL::W::Player::Ptr player) const = 0;

					/**
					 * Drive some actual players.
					 */
					virtual void execute() = 0;

				protected:
					AI::HL::W::World& world;
					AI::HL::W::Player::Ptr player;

				private:
					std::time_t start_time;
					const double timeout;

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

#endif

