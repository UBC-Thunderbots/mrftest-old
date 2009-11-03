#ifndef AI_TACTIC_KICK_H
#define AI_TACTIC_KICK_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include "ai/tactic.h"
#include "ai/tactic/turn.h"

//
// 
//
class kick : public tactic {
	public:
		//
		// A pointer to a kick tactic.
		//
		typedef Glib::RefPtr<kick> ptr;

		//
		// Constructs a new kick tactic.
		//
		kick(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void update();

		//
		// Sets the target position for this kick tactic.
		//
		void set_target(const point& p);	

		//
		// Sets rather the player chips the ball or kicks the ball.
		//
		void set_chip(const bool& chip);

	protected:		

		// Target position
		point the_target;

		// Holds if this tactic should chip the ball rather than simply kicking it
		bool should_chip;

		turn::ptr turn_tactic;

};

#endif

