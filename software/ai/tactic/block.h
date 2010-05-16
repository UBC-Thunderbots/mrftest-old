#ifndef AI_TACTIC_BLOCK_H
#define AI_TACTIC_BLOCK_H

#include "ai/tactic/move.h"

//
// 
//
class block : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<block> ptr;

		//
		// Constructs a new block tactic. 
		//
		block(player::ptr player, world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Sets the target for the block tactic.
		//
		void set_target(player::ptr target);	

	protected:
		const player::ptr the_player;

		player::ptr the_target;		

		move move_tactic;

};

#endif

