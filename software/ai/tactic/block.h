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
		// A pointer to this Tactic.
		//
		typedef RefPtr<Block> ptr;

		//
		// Constructs a new Block Tactic. 
		//
		Block(Player::ptr player, World::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target for the Block Tactic.
		 */
		void set_target(Robot::ptr target);	

	protected:
		Robot::ptr target;
		World::ptr the_world;
};

#endif

