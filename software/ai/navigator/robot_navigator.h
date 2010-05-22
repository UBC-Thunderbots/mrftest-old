#ifndef AI_NAVIGATOR_ROBOT_NAVIGATOR_H
#define AI_NAVIGATOR_ROBOT_NAVIGATOR_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"

/**
 * Note about the default behaviour:
 * - Does not have any clipping (all flags are off).
 * - Always orient towards the ball.
 *
 * Thus at every tick:
 * - call set_position to change the position
 * - call set_orientation to change orientation
 */
class robot_navigator : public noncopyable {
	public:
		robot_navigator(player::ptr player, world::ptr world);

		void tick();

		/**
		 * Sets the desired location for this time step.
		 * You have to call this function for every tick.
		 */
		void set_position(const point& position);

		/**
		 * Normally the navigator sets the robot orientation to be towards the ball.
		 * Use this if you want to override this behaviour.
		 * This only sets the desired orientation for one timestep.
		 * You have to call this function every timestep.
		 * \param orientation
		 */
		void set_orientation(const double& orientation);

#warning Refactor flags
		void set_robot_avoid_ball_amount(int amount);   

		bool robot_avoids_ball() const {
			return avoid_ball;
		}

		void set_robot_avoids_ball(bool avoid) {
			avoid_ball = avoid;
		}

		bool robot_avoids_goal();
		void set_robot_avoids_goal(bool avoid);

		bool robot_stays_on_own_half();
		void set_robot_stays_on_own_half(bool avoid);

		bool robot_stays_away_from_opponent_goal();
		void set_robot_stays_away_from_opponent_goal(bool avoid);

	private:
		bool check_vector(const point& start, const point& dest, const point& direction) const;
		double get_avoidance_factor() const;

		const player::ptr the_player;
		const world::ptr the_world;

		// Has destination and orientation been set?
		bool position_initialized;
		bool orientation_initialized;

		point target_position;
		double target_orientation;

		// ALL THIS FUNCTION BELOW HERE SHOULD BE REFACTORED
		bool avoid_ball;
		bool avoid_goal;
		double avoid_goal_amount;
		bool robot_stays_on_half;
		bool robot_avoids_opponent_goal;
};

#endif

