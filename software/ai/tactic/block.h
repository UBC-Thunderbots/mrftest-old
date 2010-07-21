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
		typedef RefPtr<Block> Ptr;

		//
		// Constructs a new Block Tactic. 
		//
		Block(Player::Ptr player, World::Ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target for the Block Tactic.
		 */
		void set_target(Robot::Ptr target);	

	protected:
		Robot::Ptr target;
		World::Ptr the_world;
};

#endif

