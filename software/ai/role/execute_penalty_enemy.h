#ifndef AI_ROLE_EXECUTE_PENALTY_ENEMY_H
#define AI_ROLE_EXECUTE_PENALTY_ENEMY_H

#include "ai/role/role.h"

/**
 * Gets the robots to go to their execute_penalty_enemy positions.
 */
class execute_penalty_enemy : public role {
	public:
		/**
		 * A pointer to a execute_penalty_enemy role.
		 */
		typedef Glib::RefPtr<execute_penalty_enemy> ptr;

		/**
		 * Constructs a new execute_penalty_enemy role.
		 *
		 * \param world the world
		 */
		execute_penalty_enemy(world::ptr world);

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

