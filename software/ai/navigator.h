#ifndef AI_NAVIGATION_H
#define AI_NAVIGATION_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/player.h"

//
// A navigator manages movement of a single robot to a target.
//
class navigator : public virtual byref {
	public:
		//
		// A pointer to a navigator.
		//
		typedef Glib::RefPtr<navigator> ptr;

		//
		// Constructs a new navigator. Call this constructor from subclass constructors.
		//
		navigator(player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		virtual void update() = 0;

	protected:
		//
		// The player being navigated.
		//
		const player::ptr the_player;
};

#endif

