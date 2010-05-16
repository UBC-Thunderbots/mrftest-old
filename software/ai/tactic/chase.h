#ifndef AI_TACTIC_CHASE_H
#define AI_TACTIC_CHASE_H

#include "ai/tactic/move.h"
#include "geom/point.h"

//
// 
//
class chase : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<chase> ptr;

		//
		// Set a target that robot would like to take ball after gaining possesion
		//
#warning this function is not implemented in chase.cpp
		void set_target(point target);

		//
		// Constructs a new chase tactic. 
		//
		chase(player::ptr player, world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:
		const player::ptr the_player;

		const world::ptr the_world;

		move move_tactic;
		
		point target;

};

#endif

