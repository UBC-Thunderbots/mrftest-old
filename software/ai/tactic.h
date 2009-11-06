#ifndef AI_TACTIC_H
#define AI_TACTIC_H

#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"

//
// A tactic controls the operation of a single player doing some activity.
//
class tactic : public byref, public sigc::trackable {
	public:
		//
		// A pointer to a tactic.
		//
		typedef Glib::RefPtr<tactic> ptr;

		//
		// Constructs a new tactic. Call this from subclass constructor.
		//
		tactic(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : the_ball(ball), the_field(field), the_team(team), the_player(player) {
		}

		//
		// Runs the tactic for one time tick. It is expected that the tactic
		// will examine the robot for which it is responsible, determine how it
		// wishes that robot to move, and then do one of the following:
		//
		//  If this tactic is layered on top of another tactic, call
		//  tactic::tick() on that lower-level tactic, OR
		//
		//  If this tactic is the bottom-level tactic and is layered on top of a
		//  navigator, call navigator::set_point() and then navigator::tick(),
		//  OR
		//
		//  If this tactic is the bottom-level tactic and does not use a
		//  navigator, call player::move().
		//
		virtual void tick() = 0;

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

