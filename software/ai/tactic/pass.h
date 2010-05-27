#ifndef AI_TACTIC_PASS_H
#define AI_TACTIC_PASS_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include "ai/tactic/tactic.h"

/**
 * Tactic to pass to another player.
 */
class pass : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<pass> ptr;

		/**
		 * Constructs a new pass tactic with receiver set.
		 */
		pass(player::ptr player, world::ptr world, player::ptr receiver);

		/**
		 * Runs the AI for one time tick.
		 */
		void tick();	

	protected:
		const world::ptr the_world;
		
		/**
		 * Pointer to the receiver of the pass.
		 */
		const player::ptr the_receiver;		
};

#endif

