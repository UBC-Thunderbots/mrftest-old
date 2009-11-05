#ifndef AI_TACTIC_CHASE_H
#define AI_TACTIC_CHASE_H

#include "ai/tactic.h"
#include "ai/tactic/move.h"

//
// 
//
class chase : public tactic {
	public:

		//
		// Constructs a new chase tactic. 
		//
		chase(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void update();	

	protected:

		move::ptr move_tactic;

};

#endif

