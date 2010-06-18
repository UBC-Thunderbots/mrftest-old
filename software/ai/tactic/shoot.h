#ifndef AI_TACTIC_SHOOT_H
#define AI_TACTIC_SHOOT_H

#include "ai/world/world.h"
#include "ai/tactic/tactic.h"

/**
 * If in possesion of the ball, tries to shoot to the goal.
 * If some else in the team has the ball, be ready to receive it.
 * Otherwise, chase the ball.
 *
 * Rules for forced shooting
 * - if open angle found to goal, shoot it
 * - if a friendly can see the goal better, pass
 * - shoot to the side of some enemy robots.
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

		/**
		 * switches pivot mode on and off.
		 */
		void toggle_pivot(const bool& b) {
			use_pivot == b;
		}

		/**
		 * force to kick somewhere very random.
		 */
		void force() {
			forced = true;
		}

	protected:
		const world::ptr the_world;
		bool forced;
		bool use_pivot;

};

#endif

