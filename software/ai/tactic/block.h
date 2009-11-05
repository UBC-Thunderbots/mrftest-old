#ifndef AI_TACTIC_BLOCK_H
#define AI_TACTIC_BLOCK_H

#include "ai/tactic.h"
#include "ai/tactic/move.h"

//
// 
//
class block : public tactic {
	public:

		//
		// Constructs a new block tactic. 
		//
		block(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void update();

		//
		// Sets the target for the block tactic.
		//
		void set_target(player::ptr target);	

	protected:

		player::ptr the_target;		

		move::ptr move_tactic;

};

#endif

