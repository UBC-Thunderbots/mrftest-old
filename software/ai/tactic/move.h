#ifndef AI_TACTIC_MOVE_H
#define AI_TACTIC_MOVE_H

#include "ai/navigator/robot_navigator.h"
#include "ai/tactic/tactic.h"

/**
 * A wrapper around robot navigator.
 */
class move : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<move> ptr;

		/**
		 * Standard constructor.
		 */
		move(player::ptr player, world::ptr world);

		/**
		 * Most usage of move tactic only sets position and should thus justify existence of this overloaded constructor.
		 * \param position Moves the robot to this position.
		 */
		move(player::ptr player, world::ptr world, const point& position);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target position for this move tactic
		 */
		void set_position(const point& p) {
			target_position = p;
			position_initialized = true;
		}

		/**
		 * Sets the target orientation for this move tactic
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

#warning TODO: refactor
		//
		//make the move tactic avoid the ball
		//
		void set_avoid_ball(bool avoid);

	protected:		
		const player::ptr the_player;
		double target_orientation;
		robot_navigator navi;

		point target_position;
		bool position_initialized;
		bool orientation_initialized;

		// TODO: refactor
		bool avoid_ball;
};

#endif

