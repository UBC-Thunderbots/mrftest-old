#ifndef AI_TACTIC_PASS_CHASE_H
#define AI_TACTIC_PASS_CHASE_H

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
class chase : public tactic {
	public:

		//
		// Constructs a new pass receive tactic. 
		//
		chase(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void update();	

	protected:

};

#endif

