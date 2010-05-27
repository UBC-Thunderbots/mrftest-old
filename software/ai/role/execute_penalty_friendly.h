#ifndef AI_ROLE_EXECUTE_PENALTY_FRIENDLY_H
#define AI_ROLE_EXECUTE_PENALTY_FRIENDLY_H

#include "ai/role/role.h"

/**
 * Gets the robots to go to their execute_penalty_friendly positions.
 */
class execute_penalty_friendly : public role {
	public:
		/**
		 * A pointer to a execute_penalty_friendly role.
		 */
		typedef Glib::RefPtr<execute_penalty_friendly> ptr;

		/**
		 * Constructs a new execute_penalty_friendly role.
		 *
		 * \param world the world
		 */
		execute_penalty_friendly(world::ptr world);

		/**
		 * Runs the AI for one time tick.
		 */
		void tick();

		/**
		 * Handles changes to the robot membership.
		 */
		void robots_changed();

	protected:
		/**
		 * The shooter assigned to kick the penalty shot.
		 */
		player::ptr the_shooter;

	private:
		const world::ptr the_world;

		/**
		 * The distance between the penalty mark and the mid point between the two goal posts as described by the rules.
		 */
		const static double PENALTY_MARK_LENGTH = 0.45;
};

#endif

