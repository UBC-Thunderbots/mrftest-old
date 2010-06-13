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
 * - call set_flags to set flags.
 */
class robot_navigator : public noncopyable {
	public:
		robot_navigator(player::ptr player, world::ptr world);

		void tick();

		/**
		 * Sets the desired location for this time step.
		 * You have to call this function for every tick.
		 */
		void set_position(const point& position) {
			position_initialized = true;
			target_position = position;
		}

		/**
		 * Normally the navigator sets the robot orientation to be towards the ball.
		 * Use this if you want to override this behaviour.
		 * This only sets the desired orientation for one timestep.
		 * You have to call this function every timestep.
		 * \param orientation
		 */
		void set_orientation(const double& orientation) {
			orientation_initialized = true;
			target_orientation = orientation;
		}

		/**
		 * Turns on dribbler at minimal speed and be ready to dribble to receive the ball.
		 * You need to call this every tick.
		 * I don't think you ever want to turn this off once you turn it on.
		 */
		void set_dribbler() {
			need_dribble = true;
		}

		/**
		 * Sets flags for this tick.
		 * Will be unset after end of a tick().
		 */
		void set_flags(const unsigned int& f) {
			flags |= f;
		}

	private:
	
		point get_inbounds_point(point dst);
		point force_defense_len(point dst);
		point force_offense_len(point dst);
		point clip_defense_area(point dst);
		point clip_offense_area(point dst);
		bool check_vector(const point& start, const point& dest, const point& direction) const;
		double get_avoidance_factor() const;

		point clip_circle(point circle_centre, double circle_radius, point dst);

		const player::ptr the_player;
		const world::ptr the_world;

		// Has destination and orientation been set?
		bool position_initialized;
		bool orientation_initialized;

		// The flags
		unsigned int flags;

		point target_position;
		double target_orientation;
		bool need_dribble;
};

#endif

