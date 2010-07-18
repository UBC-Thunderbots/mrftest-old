#ifndef AI_ROLE_EXECUTE_KICKOFF_FRIENDLY_H
#define AI_ROLE_EXECUTE_KICKOFF_FRIENDLY_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"

/**
 * Executes the kickoff for the kicking robot.
 */
class ExecuteKickoffFriendly : public Role {
	public:
		/**
		 * A pointer to a ExecuteKickoffFriendly Role.
		 */
		typedef Glib::RefPtr<ExecuteKickoffFriendly> ptr;

		/**
		 * Constructs a new ExecuteKickoffFriendly Role.
		 *
		 * \param world the world
		 */
		ExecuteKickoffFriendly(World::ptr world);

		//
		// True if kicker has made contact with the ball.
		//
		#warning roles do not persist, so variables cannot be stored!
		bool contacted_ball;

		//
		// Tells the kicker to move away from the ball.
		//
		void avoid_ball(int);

		//
		// Tells the kicker to kick the ball (slightly) forward.
		//
		void kick_ball(int);

		//
		// Tells the kicker to take posession of the ball.
		//
		void chase_ball(int);

		/**
		 * Runs the AI for one time tick.
		 */
		void tick();

		/**
		 * Handles changes to the robot membership.
		 */
		void robots_changed();

	private:
		const World::ptr the_world;
		std::vector<Tactic::ptr> the_tactics;
};

#endif

