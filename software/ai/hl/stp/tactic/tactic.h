#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/stp/world.h"
#include "util/byref.h"
#include "util/registerable.h"

#include <set>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * A tactic is a layer in the STP paradigm.
				 * See STP paper section 4.1, and the wiki.
				 *
				 * A tactic is an action and has verb names.
				 *
				 * It runs every tick.
				 * A subclass shall derive select(), execute(),
				 * optionally player_changed().
				 *
				 * Important tactics that deal with the ball are called active tactics.
				 * Only one such tactic is active at any given time.
				 * Other tactics must wait for the active tactic to finish.
				 * An active tactic must override done().
				 *
				 * To prevent rapid fluctuation of parameters,
				 * hysteresis (thresholding) is recommended.
				 */
				class Tactic : public ByRef {
					public:
						typedef RefPtr<Tactic> Ptr;

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
						 * Selects a player from the set.
						 * A subclass must implement this function.
						 *
						 * \param[in] players a set of players to choose from
						 *
						 * \return a player to be used by this tactic
						 */
						virtual Player::Ptr select(const std::set<Player::Ptr> &players) const;

						/**
						 * Changes the player associated with this tactic.
						 * A subclass should not call this.
						 */
						void set_player(Player::Ptr p);

						/**
						 * The main execution of this tactic.
						 * This function runs every tick.
						 * A subclass must implement this function.
						 */
						virtual void execute() = 0;

						/**
						 * A string description of this tactic.
						 */
						virtual std::string description() const;

					protected:
						const World &world;
						Player::Ptr player;

						/**
						 * Constructor for tactic.
						 *
						 * \param [in] active indicates if this is an active tactic.
						 */
						Tactic(const World &world, bool active = false);

						/**
						 * Destructor.
						 */
						~Tactic();

						/**
						 * Triggerred when the player associated changes.
						 */
						virtual void player_changed();

					private:
						const bool active_;
				};
			}
		}
	}
}

#endif

