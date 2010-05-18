#ifndef AI_TACTIC_SHOOT_H
#define AI_TACTIC_SHOOT_H

#include "ai/tactic/chase.h"
#include "ai/tactic/kick.h"

//
// A tactic controls the operation of a single player doing some activity.
//
class shoot : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<shoot> ptr;

		//
		// Constructs a new pass receive tactic. 
		//
		shoot(player::ptr player, world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:
		const player::ptr the_player;

		const world::ptr the_world;
	
		chase chase_tactic;

		kick kick_tactic;

};

#endif

