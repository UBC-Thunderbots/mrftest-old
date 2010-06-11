#ifndef AI_TACTIC_H
#define AI_TACTIC_H

#include "util/byref.h"
#include "ai/world/player.h"
#include <glibmm.h>

/**
 * A tactic controls the operation of one single player doing a very specific activity.
 * Tactics are NOT persistent.
 */
class tactic : public byref {
	public:
		/**
		 * A pointer to a tactic.
		 */
		typedef Glib::RefPtr<tactic> ptr;

		/**
		 * Runs the tactic for one time tick. It is expected that the tactic
		 * will examine the robot for which it is responsible, determine how it
		 * wishes that robot to move, and then do one of the following:
		 *
		 * Most tactics should only be a layer on top of a robot navigator.
		 * Such a tactic may need to call navigator::set_position()
		 * and navigator::set_flags(). navigator::tick() MUST be called
		 * before this function ends. OR
		 *
		 * More complex tactics can be layered on top of another tactic, call
		 * tactic::set_flags() and then tactic::tick() on that lower-level tactic.
		 *
		 * Tactic should NEVER call player::move() directly because doing so 
		 * will ignore any flags that are set.
		 */
		virtual void tick() = 0;

		/**
		 * Set flags that restrict the movement of the robots. Such flags
		 * are used to enforce rules, such as avoiding the enemy defence area.
		 * Flags are permanent once set.
		 */
		void set_flags(const unsigned int& f) {
			flags |= f;
		}

	protected:
		/**
		 * Constructor, flags set to 0 by default.
		 */
		tactic() : flags(0) {
		}

		explicit tactic(player::ptr player) : flags(0), the_player(player) {
		}

		explicit tactic(player::ptr player, const unsigned int& f) : flags(f), the_player(player) {
		}

		unsigned int flags;
		const player::ptr the_player;
};

#endif

