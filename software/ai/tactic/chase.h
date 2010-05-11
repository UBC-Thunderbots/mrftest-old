#ifndef AI_TACTIC_CHASE_H
#define AI_TACTIC_CHASE_H

#include "ai/tactic.h"
#include "ai/tactic/move.h"
#include "geom/point.h"

//
// 
//
class chase : public tactic {
	public:
		//
		// A pointer to a kick tactic.
		//
		typedef Glib::RefPtr<chase> ptr;
		//
		// Set a target that robot would like to take ball after gaining possesion
		//
		void set_target(point target);

		//
		// Constructs a new chase tactic. 
		//
		chase(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:

		move::ptr move_tactic;
		
		point target;

};

#endif

