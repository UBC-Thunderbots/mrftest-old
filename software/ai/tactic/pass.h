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
		/**
		 * Constructs a new Pass Tactic with receiver set.
		 */
		Pass(RefPtr<Player> player, RefPtr<World> world, RefPtr<Player> receiver);

		/**
		 * Runs the AI for one time tick.
		 */
		void tick();	

	protected:
		const RefPtr<World> the_world;
		
		/**
		 * Pointer to the receiver of the pass.
		 */
		const RefPtr<Player> the_receiver;		
};

#endif

