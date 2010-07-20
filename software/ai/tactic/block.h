#ifndef AI_TACTIC_BLOCK_H
#define AI_TACTIC_BLOCK_H

#include "ai/tactic/tactic.h"
#include "ai/navigator/robot_navigator.h"

/**
 * Calculates an optimal defensive point to blocks an enemy AI.
 * TODO: This Tactic does not look right.
 */
class Block : public Tactic {
	public:
		//
		// Constructs a new Block Tactic. 
		//
		Block(RefPtr<Player> player, RefPtr<World> world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target for the Block Tactic.
		 */
		void set_target(RefPtr<Robot> target);	

	protected:
		RefPtr<Robot> target;
		RefPtr<World> the_world;
};

#endif

