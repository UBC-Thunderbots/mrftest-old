#ifndef AI_TACTIC_SHOOT_H
#define AI_TACTIC_SHOOT_H

#include "ai/tactic.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/kick.h"

//
// A tactic controls the operation of a single player doing some activity.
//
class shoot : public tactic {
	public:

		//
		// Constructs a new pass receive tactic. 
		//
		shoot(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:
	
		chase::ptr chase_tactic;

		kick::ptr kick_tactic;

};

#endif

