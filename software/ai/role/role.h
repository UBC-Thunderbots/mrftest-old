#ifndef AI_ROLE_H
#define AI_ROLE_H

#include "ai/world/world.h"
#include "util/byref.h"
#include "ai/flags.h"
#include <vector>
#include <glibmm.h>

/**
 * A Role manages the operation of a small group of players.
 */
class Role2 : public ByRef, public sigc::trackable {
	public:
		/**
		 * A pointer to a Role.
		 */
		typedef Glib::RefPtr<Role2> ptr;

		/**
		 * Runs the Role for one time tick. It is expected that the Role will
		 * examine the robots for which it is responsible, determine if they
		 * need to be given new tactics, and then call Tactic::tick() for all
		 * the tactics under this Role.
		 *
		 * It is possible that the set of robots controlled by the Tactic may
		 * change between one tick and the next. The Role must be ready to deal
		 * with this situation, and must be sure to destroy any tactics
		 * controlling robots that have gone away. This situation can be
		 * detected by implementing robots_changed(), which will be called
		 * whenever the set of robots changes.
		 *
		 * \param[in] overlay the visualizer's overlay on which to draw
		 * graphical information.
		 */
		virtual void tick(Cairo::RefPtr<Cairo::Context> overlay) = 0;

		/**
		 * Called each time the set of robots for which the Role is responsible
		 * has changed. It is expected that the Role will examine the robots and
		 * determine if any changes need to be made.
		 */
		virtual void robots_changed() = 0;
		
		/**
		 * Sets the robots controlled by this Role.
		 *
		 * \param[in] r the robots the Role should control.
		 */
		void set_robots(const std::vector<Player::ptr> &r) {
			robots = r;
			robots_changed();
		}
		
		/**
		 * Sets the one and only robot controlled by this Role.
		 *
		 * \param[in] r the robot the Role should control.
		 */
		void set_robot(const Player::ptr &r) {
			robots.clear();
			robots.push_back(r);
			robots_changed();
		}
		
		/**
		 * Removes all robots from this Role.
		 */
		void clear_robots() {
			robots.clear();
			robots_changed();
		}

	protected:
		/**
		 * The robots that this Role controls.
		 */
		std::vector<Player::ptr> robots;
};

/**
 * A compatibility shim for roles that do not present a visual overlay.
 */
class Role : public Role2 {
	public:
		/**
		 * A pointer to a Role.
		 */
		typedef Glib::RefPtr<Role> ptr;

		/**
		 * Runs the Role for one time tick. It is expected that the Role will
		 * examine the robots for which it is responsible, determine if they
		 * need to be given new tactics, and then call Tactic::tick() for all
		 * the tactics under this Role.
		 *
		 * It is possible that the set of robots controlled by the Tactic may
		 * change between one tick and the next. The Role must be ready to deal
		 * with this situation, and must be sure to destroy any tactics
		 * controlling robots that have gone away. This situation can be
		 * detected by implementing robots_changed(), which will be called
		 * whenever the set of robots changes.
		 */
		virtual void tick() = 0;

	private:
		void tick(Cairo::RefPtr<Cairo::Context>) {
			tick();
		}
};

#endif

