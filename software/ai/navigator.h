#ifndef AI_NAVIGATION_H
#define AI_NAVIGATION_H

#include "util/byref.h"
#include "world/player.h"
#include "world/field.h"
#include "world/ball.h"
#include "world/team.h"
#include <glibmm.h>
#include <sigc++/sigc++.h>

//
// A navigator manages movement of a single robot to a target.
//
class navigator : public byref, public sigc::trackable {
	public:
		//
		// A pointer to a navigator.
		//
		typedef Glib::RefPtr<navigator> ptr;

		//
		// Constructs a new navigator. Call this constructor from subclass constructors.
		//
		navigator(player::ptr player,  field::ptr field, ball::ptr ball, team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		virtual void update() = 0;

		//
		// Instruct the navigator to move the player to a point.
		//
		virtual void set_point(const point& destination) = 0;

	protected:
		//
		// The player being navigated.
		//
		const player::ptr the_player;
		const field::ptr the_field;
		const ball::ptr the_ball;
		const team::ptr the_team;
};

#endif

