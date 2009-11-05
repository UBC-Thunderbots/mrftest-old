#ifndef AI_TACTIC_BLOCK_H
#define AI_TACTIC_BLOCK_H

#include "ai/tactic.h"

//
// 
//
class turn : public tactic {
	public:
		//
		// A pointer to a turn tactic.
		//
		typedef Glib::RefPtr<turn> ptr;

		//
		// Constructs a new block tactic. 
		//
		turn(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void update();

		//
		// Sets the direction for the turn tactic. Takes in a point and turns towards that point.
		//
		void set_direction(const point& dir);	

	protected:

		point the_direction;				

};

#endif

