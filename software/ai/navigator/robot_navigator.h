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

		/**
		 * Conditions that the robot must obey.
		 * By default, nothing is set.
		 */
		enum {
			clip_play_area         = 0x0001, /**< enum Force robot to stay in play area. */
			avoid_ball_near        = 0x0002, /**< enum Avoid ball for pivoting (probably used for free kick). */
			avoid_ball_stop        = 0x0004, /**< enum Avoid ball when stop is in play. */
			avoid_friendly_defence = 0x0008, /**< enum Avoid friendly defence area. */
			avoid_enemy_defence    = 0x0010, /**< enum Avoid enemy defence area. */
			stay_own_half          = 0x0020, /**< enum Stay in your own half. */
			penalty_kick_friendly  = 0x0040, /**< enum Neither goalie nor kicker. Stay away at certain boundary. */
			penalty_kick_enemy     = 0x0080, /**< enum Neither goalie nor kicker. Stay away at certain boundary. */
		};

		/**
		 * Sets flags permanently.
		 * There is no method to unset a flag.
		 */
		void set_flags(const unsigned int& f) {
			flags |= f;
		}

	private:
	
		bool dst_ok(point dst);
	
		point get_inbounds_point(point dst);
		point force_defense_len(point dst);
		point force_offense_len(point dst);
		point clip_defense_area(point dst);
		point clip_offense_area(point dst);
		bool check_vector(const point& start, const point& dest, const point& direction) const;
		double get_avoidance_factor() const;

		const player::ptr the_player;
		const world::ptr the_world;

		// Has destination and orientation been set?
		bool position_initialized;
		bool orientation_initialized;

		// The flags
		unsigned int flags;

		point target_position;
		double target_orientation;
};

#endif

