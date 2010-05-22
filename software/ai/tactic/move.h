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

		//
		// Constructs a new move tactic.
		//
		move(player::ptr player, world::ptr world);

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
		 * If this is not called, robot will face towards the ball.
		 */
		void set_orientation(const double& d) {
			target_orientation = d;
			orientation_initialized = true;
		}

#warning TODO: refactor
		//
		//make the move tactic avoid the ball
		//
		void set_avoid_ball(bool avoid);
		
	protected:		
		const player::ptr the_player;
		point target_position;
		double target_orientation;
		robot_navigator navi;

		bool position_initialized;
		bool orientation_initialized;

		bool avoid_ball;
};

#endif

