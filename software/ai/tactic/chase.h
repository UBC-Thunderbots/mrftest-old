#ifndef AI_TACTIC_CHASE_H
#define AI_TACTIC_CHASE_H

#include "ai/tactic/tactic.h"
#include "ai/navigator/robot_navigator.h"

/**
 * Chase the ball.
 */
class chase : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<chase> ptr;

		//
		// Constructs a new chase tactic. 
		//
		chase(player::ptr player, world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:
		const world::ptr the_world;
		robot_navigator navi;
};

#endif

