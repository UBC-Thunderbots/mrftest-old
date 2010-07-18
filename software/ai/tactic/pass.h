#ifndef AI_TACTIC_PASS_H
#define AI_TACTIC_PASS_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include "ai/tactic/tactic.h"

/**
 * Tactic to pass to another player.
 */
class Pass : public Tactic {
	public:
		//
		// A pointer to this Tactic.
		//
		typedef Glib::RefPtr<Pass> ptr;

		/**
		 * Constructs a new Pass Tactic with receiver set.
		 */
		Pass(Player::ptr player, World::ptr world, Player::ptr receiver);

		/**
		 * Runs the AI for one time tick.
		 */
		void tick();	

	protected:
		const World::ptr the_world;
		
		/**
		 * Pointer to the receiver of the pass.
		 */
		const Player::ptr the_receiver;		
};

#endif

