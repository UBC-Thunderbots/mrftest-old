#ifndef AI_TACTIC_PASS_H
#define AI_TACTIC_PASS_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include "ai/tactic.h"
#include "ai/tactic/pass_mode.h"
#include "ai/tactic/kick.h"

//
// A tactic controls the operation of a single player doing some activity.
//
class pass : public tactic {
	public:

		//
		// Constructs a new pass tactic.
		//
		pass(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Sets the receiver for this pass tactic.
		//
		void set_receiver(player::ptr receiver);

		//
		// Runs the AI for one time tick.
		//
		void update();	

	protected:
		
		kick::ptr kick_tactic;

		// Pointer to the receiver of the pass.
		player::ptr the_receiver;
		
		// Passing mode
		pass_mode mode;

		//
		// Calculates the target based on the passing mode and the receiver.
		//
		point calculate_target();
};

#endif

