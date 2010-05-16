#ifndef AI_ROLE_EXECUTE_KICKOFF_ENEMY_H
#define AI_ROLE_EXECUTE_KICKOFF_ENEMY_H

#include "ai/role/role.h"

/**
 * Gets the robots to go to their execute_kickoff_enemy positions.
 */
class execute_kickoff_enemy : public role {
	public:
		/**
		 * A pointer to a execute_kickoff_enemy role.
		 */
		typedef Glib::RefPtr<execute_kickoff_enemy> ptr;

		/**
		 * Constructs a new execute_kickoff_enemy role.
		 *
		 * \param world the world
		 */
		execute_kickoff_enemy(world::ptr world);

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

