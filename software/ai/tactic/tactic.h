#ifndef AI_TACTIC_H
#define AI_TACTIC_H

#include "util/byref.h"
#include "ai/world/player.h"
#include <glibmm.h>

/**
 * A Tactic controls the operation of one single player doing a very specific activity.
 * Tactics are NOT persistent.
 */
class Tactic : public ByRef {
	public:
		/**
		 * A pointer to a Tactic.
		 */
		typedef Glib::RefPtr<Tactic> ptr;

		/**
		 * Runs the Tactic for one time tick. It is expected that the Tactic
		 * will examine the robot for which it is responsible, determine how it
		 * wishes that robot to move, and then do one of the following:
		 *
		 * Most tactics should only be a layer on top of a robot navigator.
		 * Such a tactic may need to call navigator::set_position()
		 * and navigator::set_flags(). navigator::tick() MUST be called
		 * before this function ends. OR
		 *
		 * More complex tactics can be layered on top of another tactic, call
		 * Tactic::set_flags() and then Tactic::tick() on that lower-level Tactic.
		 *
		 * Tactic should NEVER call Player::move() directly because doing so 
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
		explicit Tactic(const Player::ptr& player) : flags(0), player(player) {
		}

		explicit Tactic(const Player::ptr& player, const unsigned int& f) : flags(f), player(player) {
		}

		unsigned int flags;
		const Player::ptr player;
};

#endif

