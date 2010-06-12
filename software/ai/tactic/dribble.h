#ifndef AI_TACTIC_DRIBBLE_H
#define AI_TACTIC_DRIBBLE_H

#include "ai/navigator/robot_navigator.h"
#include "ai/tactic/tactic.h"

/**
 * Unlike move, this dribbles the ball as well.
 * Also, if the robot loses sense of the ball, tries to pick it up.
 */
class dribble : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<dribble> ptr;

		/**
		 * Standard constructor.
		 */
		dribble(player::ptr player, world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target position for this dribble tactic
		 */
		void set_position(const point& p) {
			target_position = p;
			position_initialized = true;
		}

		/**
		 * Gets whether the position is set for this dribble tactic.
		 */
		bool is_position_set() {
			return position_initialized;
		}

		/**
		 * Sets the target orientation for this dribble tactic
		 */
		void set_orientation(const double& d) {
			target_orientation = d;
			orientation_initialized = true;
		}

		/**
		 * Makes the robot face the ball.
		 */
		void unset_orientation() {
			orientation_initialized = false;
		}

	protected:		
		const world::ptr the_world;
		robot_navigator navi;

		point target_position;
		double target_orientation;

		bool position_initialized;
		bool orientation_initialized;
};

#endif

