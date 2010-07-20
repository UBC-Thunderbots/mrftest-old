#ifndef AI_ROLE_EXECUTE_FREE_KICK_H
#define AI_ROLE_EXECUTE_FREE_KICK_H

#include "ai/role/role.h"

/**
 * Execute an indirect free kick.
 * Should only contain one robot.
 */
class ExecuteIndirectFreeKick : public Role {
	public:

		ExecuteIndirectFreeKick(RefPtr<World> world);

		void tick();

		void players_changed();

	private:
		const RefPtr<World> the_world;
};

/**
 * Execute a direct free kick.
 * Should only contain one robot.
 */
class ExecuteDirectFreeKick : public Role {
	public:

		ExecuteDirectFreeKick(RefPtr<World> world);

		void tick();

		void players_changed();

	private:
		const RefPtr<World> the_world;
};

#endif

