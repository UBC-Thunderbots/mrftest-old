#ifndef AI_ROLE_EXECUTE_DIRECT_FREE_KICK_FRIENDLY_H
#define AI_ROLE_EXECUTE_DIRECT_FREE_KICK_FRIENDLY_H

#include "ai/role/role.h"

/**
 * Gets the robots to go to their execute_direct_free_kick_friendly positions.
 */
class execute_direct_free_kick_friendly : public role {
	public:
		/**
		 * A pointer to a execute_direct_free_kick_friendly role.
		 */
		typedef Glib::RefPtr<execute_direct_free_kick_friendly> ptr;

		/**
		 * Constructs a new execute_direct_free_kick_friendly role.
		 *
		 * \param world the world
		 */
		execute_direct_free_kick_friendly(world::ptr world);

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

