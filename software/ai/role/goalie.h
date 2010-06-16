#ifndef AI_ROLE_GOALIE_H
#define AI_ROLE_GOALIE_H

#include "ai/role/role.h"

/**
 * Defend the goal area with the robot's life.
 * If in possesion of a ball, passes to a friendly unit
 * WILL NOT chase the ball.
 */
class goalie : public role {
	public:
		//
		// A pointer to a goalie role.
		//
		typedef Glib::RefPtr<goalie> ptr;

		//
		// Constructs a new goalie role.
		//
		goalie(world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:
		const world::ptr the_world;

	private:

		void run_vel_goalie(player::ptr goalie, const unsigned int& flags);

		void run_goalie_confidence(player::ptr goalie, const unsigned int& flags);

		/// Old version
		void run_goalie_old(const unsigned int& flags);

};

#endif

