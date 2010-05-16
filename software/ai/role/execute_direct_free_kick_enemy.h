#ifndef AI_ROLE_EXECUTE_DIRECT_FREE_KICK_ENEMY_H
#define AI_ROLE_EXECUTE_DIRECT_FREE_KICK_ENEMY_H

#include "ai/role/role.h"

/**
 * Gets the robots to go to their execute_direct_free_kick_enemy positions.
 */
class execute_direct_free_kick_enemy : public role {
	public:
		/**
		 * A pointer to a execute_direct_free_kick_enemy role.
		 */
		typedef Glib::RefPtr<execute_direct_free_kick_enemy> ptr;

		/**
		 * Constructs a new execute_direct_free_kick_enemy role.
		 *
		 * \param world the world
		 */
		execute_direct_free_kick_enemy(world::ptr world);

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

