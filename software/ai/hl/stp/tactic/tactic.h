#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/stp/world.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <cairomm/cairomm.h>
#include <glibmm.h>
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
				 * Every subclass must implement execute().
				 * Non-goalie tactics must implement select().
				 * Active tactics must implement done().
				 * Subclass may optionally implement player_changed().
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
						 * A tactic can fail if something really bad happens.
						 * Only usable by active tactic.
						 */
						virtual bool fail() const;

						/**
						 * Checks if the current tactic is an active tactic.
						 */
						bool active() const {
							return active_;
						}

						/**
						 * Selects a player from the set.
						 * A non-goalie tactic must implement this function.
						 *
						 * \param[in] players a set of players to choose from
						 *
						 * \return a player to be used by this tactic
						 */
						virtual Player::Ptr select(const std::set<Player::Ptr> &players) const = 0;

						/**
						 * Changes the player associated with this tactic.
						 */
						void set_player(Player::Ptr p);

						/**
						 * The main execution of this tactic.
						 * This function runs every tick.
						 * A subclass must implement this function.
						 */
						virtual void execute() = 0;

						/**
						 * An optional string description of this tactic.
						 */
						virtual Glib::ustring description() const;

						/**
						 * An optional function to draw extra stuff on the overlay.
						 */
						virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> context) const;

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

