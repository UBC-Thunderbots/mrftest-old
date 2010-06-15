#ifndef AI_ROLE_EXECUTE_FREE_KICK_H
#define AI_ROLE_EXECUTE_FREE_KICK_H

#include "ai/role/role.h"

/**
 * Execute an indirect free kick.
 * Should only contain one robot.
 */
class execute_indirect_free_kick : public role {
	public:

		typedef Glib::RefPtr<execute_indirect_free_kick> ptr;

		execute_indirect_free_kick(world::ptr world);

		void tick();

		void robots_changed();

	private:
		const world::ptr the_world;
};

/**
 * Execute a direct free kick.
 * Should only contain one robot.
 */
class execute_direct_free_kick : public role {
	public:

		typedef Glib::RefPtr<execute_direct_free_kick> ptr;

		execute_direct_free_kick(world::ptr world);

		void tick();

		void robots_changed();

	private:
		const world::ptr the_world;
};

#endif

