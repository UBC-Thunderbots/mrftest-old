#ifndef AI_TACTIC_SHOOT_H
#define AI_TACTIC_SHOOT_H

#include "ai/world/world.h"
#include "ai/tactic/tactic.h"

/**
 * If in possesion of the ball, tries to shoot to the goal.
 * If some else in the team has the ball, be ready to receive it.
 * Otherwise, chase the ball.
 */
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

		/*
		 * force to kick somewhere very random.
		 */
		void force() {
			forced = true;
		}

	protected:
		const world::ptr the_world;
		bool forced;

};

#endif

