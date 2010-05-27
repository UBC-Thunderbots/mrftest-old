#ifndef AI_TACTIC_BLOCK_H
#define AI_TACTIC_BLOCK_H

#include "ai/tactic/tactic.h"
#include "ai/navigator/robot_navigator.h"

/**
 * Calculates an optimal defensive point to blocks an enemy AI.
 * TODO: This tactic does not look right.
 */
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

		/**
		 * Sets the target for the block tactic.
		 */
		void set_target(robot::ptr target);	

	protected:
		robot::ptr target;
		robot_navigator navi;
};

#endif

