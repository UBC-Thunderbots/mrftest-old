#ifndef AI_ROLE_GOALIE_H
#define AI_ROLE_GOALIE_H

#include "ai/role/role.h"

/**
 * Defend the goal area with the robot's life.
 * If in possesion of a ball, passes to a friendly unit
 * WILL NOT chase the ball.
 */
class Goalie : public Role {
	public:
		//
		// Constructs a new Goalie Role.
		//
		Goalie(RefPtr<World> world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void players_changed();

	protected:
		const RefPtr<World> the_world;

	private:

		void run_vel_goalie(RefPtr<Player> goalie, const unsigned int& flags);

		void run_goalie_confidence(RefPtr<Player> goalie, const unsigned int& flags);

		/// Old version
		void run_goalie_old(const unsigned int& flags);

};

#endif

