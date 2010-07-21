#ifndef AI_ROLE_EXECUTE_FREE_KICK_H
#define AI_ROLE_EXECUTE_FREE_KICK_H

#include "ai/role/role.h"

/**
 * Execute an indirect free kick.
 * Should only contain one robot.
 */
class ExecuteIndirectFreeKick : public Role {
	public:

		typedef Glib::RefPtr<ExecuteIndirectFreeKick> ptr;

		ExecuteIndirectFreeKick(World::ptr world);

		void tick();

		void players_changed();

	private:
		const World::ptr the_world;
};

/**
 * Execute a direct free kick.
 * Should only contain one robot.
 */
class ExecuteDirectFreeKick : public Role {
	public:

		typedef Glib::RefPtr<ExecuteDirectFreeKick> ptr;

		ExecuteDirectFreeKick(World::ptr world);

		void tick();

		void players_changed();

	private:
		const World::ptr the_world;
};

#endif

