#ifndef AI_TACTIC_PASS_RECEIVE_H
#define AI_TACTIC_PASS_RECEIVE_H

#include "ai/tactic/turn.h"

//
// A tactic controls the operation of a single player doing some activity.
//
class pass_receive : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<pass_receive> ptr;

		//
		// Constructs a new pass receive tactic. 
		//
		pass_receive(player::ptr player, player::ptr passer);
		
		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:
		
		// Pointer to the passer of the pass.
		player::ptr the_passer;

		// The turn tactic used to turn towards the passer.
		turn turn_tactic;
};

#endif

