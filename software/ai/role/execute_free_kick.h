#ifndef AI_ROLE_EXECUTE_FREE_KICK_H
#define AI_ROLE_EXECUTE_FREE_KICK_H

#include "ai/role/role.h"

/**
 * Execute an indirect free kick.
 * Should only contain one robot.
 */
class ExecuteIndirectFreeKick : public Role {
	public:

		typedef RefPtr<ExecuteIndirectFreeKick> Ptr;

		ExecuteIndirectFreeKick(World::Ptr world);

		void tick();

		void players_changed();

	private:
		const World::Ptr the_world;
};

/**
 * Execute a direct free kick.
 * Should only contain one robot.
 */
class ExecuteDirectFreeKick : public Role {
	public:

		typedef RefPtr<ExecuteDirectFreeKick> Ptr;

		ExecuteDirectFreeKick(World::Ptr world);

		void tick();

		void players_changed();

	private:
		const World::Ptr the_world;
};

#endif

