#ifndef AI_ROLE_EXECUTE_KICKOFF_FRIENDLY_H
#define AI_ROLE_EXECUTE_KICKOFF_FRIENDLY_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"

/**
 * Executes the kickoff for the kicking robot.
 */
class execute_kickoff_friendly : public role {
	public:
		/**
		 * A pointer to a execute_kickoff_friendly role.
		 */
		typedef Glib::RefPtr<execute_kickoff_friendly> ptr;

		/**
		 * Constructs a new execute_kickoff_friendly role.
		 *
		 * \param world the world
		 */
		execute_kickoff_friendly(world::ptr world);

		//
		// True if kicker has made contact with the ball.
		//
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
		const world::ptr the_world;
		std::vector<tactic::ptr> the_tactics;
};

#endif

