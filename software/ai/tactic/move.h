#ifndef AI_TACTIC_MOVE_H
#define AI_TACTIC_MOVE_H

#include "ai/navigator/robot_navigator.h"
#include "ai/tactic/tactic.h"

/**
 * A wrapper around robot navigator.
 * I.e. this class exist only so that roles do not call navigator directly.
 * Therefore, me thinks that other Tactic should not have instance of this Tactic.
 */
class Move : public Tactic {
	public:
		/**
		 * Standard constructor.
		 */
		Move(RefPtr<Player> player, RefPtr<World> world);

 		/**
-		 * Overloaded constructor with flags option.
 		 */
 		Move(RefPtr<Player> player, RefPtr<World> world, const unsigned int& flags);

		/**
		 * Most usage of Move Tactic only sets position and should thus justify existence of this overloaded constructor.
		 * \param position Moves the robot to this position.
		 */
		// Move(RefPtr<Player> player, RefPtr<World> world, const unsigned int& flags, const Point& position);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target position for this Move Tactic
		 */
		void set_position(const Point& p) {
			target_position = p;
			position_initialized = true;
		}

		/**
		 * Gets whether the position is set for this Move Tactic.
		 */
		bool is_position_set() {
			return position_initialized;
		}

		/**
		 * Sets the target orientation for this Move Tactic
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
		RobotNavigator navi;

		Point target_position;
		double target_orientation;

		bool position_initialized;
		bool orientation_initialized;
};

#endif

