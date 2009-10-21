#ifndef AI_TACTIC_H
#define AI_TACTIC_H

#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include <glibmm.h>
#include <sigc++/sigc++.h>

//
// A tactic controls the operation of a single player doing some activity.
//
class tactic : public virtual byref, public virtual sigc::trackable {
	public:
		//
		// A pointer to a tactic.
		//
		typedef Glib::RefPtr<tactic> ptr;

		//
		// Constructs a new tactic. Call this from subclass constructor.
		//
		tactic(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		virtual void update() = 0;

	protected:
		//
		// The ball.
		//
		const ball::ptr the_ball;

		//
		// The field.
		//
		const field::ptr the_field;

		//
		// The team this role controls.
		//
		const controlled_team::ptr the_team;

		//
		// The player this role controls.
		//
		const player::ptr the_player;
};

#endif

