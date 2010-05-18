#ifndef AI_NAVIGATOR_ROBOT_NAVIGATOR_H
#define AI_NAVIGATOR_ROBOT_NAVIGATOR_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"

class robot_navigator : public noncopyable {
	public:
		robot_navigator(player::ptr player, world::ptr world);

		void tick();
		void set_point(const point& destination);

		void set_correction_step_size(double correction_size);  
		void set_desired_robot_orientation(double orientation);

#warning "implement or delete function soon"
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
		bool dest_initialized;//has a destination been specified?
		point curr_dest;//current destination
		double outofbounds_margin;//distance to remain from sidelines to prevent from going oob
		bool avoid_ball;
		bool avoid_goal;
		double avoid_goal_amount;
		bool robot_stays_on_half;
		bool robot_avoids_opponent_goal;

		/// Max distance of lookahead distance.
		double lookahead_max;
};

#endif

