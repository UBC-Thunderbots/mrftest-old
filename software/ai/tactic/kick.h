#ifndef AI_TACTIC_KICK_H
#define AI_TACTIC_KICK_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include "ai/tactic.h"
#include "ai/navigator.h"

//
// Just a wrapper around the move function in player.
//
class kick : public tactic {
	public:
		//
		// A pointer to a move tactic.
		//
		typedef Glib::RefPtr<kick> ptr;

		//
		// Constructs a new move tactic.
		//
		kick(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void update();

		//
		// Sets the target position for this move tactic
		//
		void set_target(const point& p);	

	protected:		

		// Target position
		point the_target;

};

#endif

