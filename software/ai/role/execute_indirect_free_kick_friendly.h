#ifndef AI_ROLE_EXECUTE_INDIRECT_FREE_KICK_FRIENDLY_H
#define AI_ROLE_EXECUTE_INDIRECT_FREE_KICK_FRIENDLY_H

#include "ai/role/role.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/pass.h"

/**
 * Gets the robots to go to their execute_indirect_free_kick_friendly positions.
 */
class execute_indirect_free_kick_friendly : public role {
	public:
		/**
		 * A pointer to a execute_indirect_free_kick_friendly role.
		 */
		typedef Glib::RefPtr<execute_indirect_free_kick_friendly> ptr;

		/**
		 * Constructs a new execute_indirect_free_kick_friendly role.
		 *
		 * \param world the world
		 */
		execute_indirect_free_kick_friendly(world::ptr world);

		/**
		 * Runs the AI for one time tick.
		 */
		void tick();

		/**
		 * Handles changes to the robot membership.
		 */
		void robots_changed();

	private:
		const world::ptr the_world;
};

#endif

