#ifndef AI_TACTIC_PASS_H
#define AI_TACTIC_PASS_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include "ai/tactic.h"

//
// A tactic controls the operation of a single player doing some activity.
//
class pass : public tactic {
	public:

		//
		// Constructs a new pass tactic.
		//
		pass(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		void set_receiver(player::ptr r) {
			receiver = r;
		}		
		
		//
		// Runs the AI for one time tick.
		//
		void update();	

	protected:
		
		// Pointer to the receiver of the pass.
		player::ptr receiver;
};

#endif

