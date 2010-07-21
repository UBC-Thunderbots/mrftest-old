#ifndef AI_ROLE_H
#define AI_ROLE_H

#include "ai/world/world.h"
#include "util/byref.h"
#include "ai/flags.h"
#include <vector>
#include <glibmm.h>

/**
 * A Role manages the operation of a group of players.
 */
class Role : public ByRef, public sigc::trackable {
	public:
		/**
		 * A pointer to a Role.
		 */
		typedef RefPtr<Role> ptr;

		/**
		 * Runs the Role for one time tick. It is expected that the Role will
		 * examine the players for which it is responsible, determine if they
		 * need to be given new tactics, and then call Tactic::tick() for all
		 * the tactics under this Role.
		 *
		 * WARNING
		 * Current architecture reassigns players every tick.
		 * A Role must be prepared for this.
		 *
		 * Old note:
		 * It is possible that the set of robots controlled by the Tactic may
		 * change between one tick and the next. The Role must be ready to deal
		 * with this situation, and must be sure to destroy any tactics
		 * controlling players that have gone away. This situation can be
		 * detected by implementing robots_changed(), which will be called
		 * whenever the set of players changes.
		 *
		 */
		virtual void tick() = 0;

		/**
		 * Sets the players controlled by this Role.
		 *
		 * \param[in] ps the players the Role should control.
		 */
		void set_players(const std::vector<Player::ptr> &ps) {
			players = ps;
			players_changed();
		}

		/**
		 * Removes all players from this Role.
		 */
		void clear_players() {
			players.clear();
			players_changed();
		}

		/**
		 * Called each time the set of players for which the Role is responsible
		 * has been reassigned.
		 * It is expected that the Role will examine the players and
		 * determine if any changes need to be made.
		 *
		 * NOTE
		 * - This function is redundant in a stateless AI because players are reassigned
		 *   every tick.
		 * - It does not matter if the set of players stay the same. As long as
		 *   players are reassigned, this function is called.
		 */
		virtual void players_changed() = 0;

	protected:
		/**
		 * The robots that this Role controls.
		 */
		std::vector<Player::ptr> players;
};

#endif
