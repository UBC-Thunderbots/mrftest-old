#ifndef AI_WORLD_WORLD_HELPER_H
#define AI_WORLD_WORLD_HELPER_H

#include "ai/world/world.h"

#include <algorithm>
#include <vector>

/**
 * Contains alot of useful functions related to the world.
 * This class is work in progress, and intended to avoid having too many static functions in ai_util.
 */
class world_helper {
	public:
		world_helper(world::ptr w) : the_world(w) {
		}

		/// Position of own goal.
		point self_center() const {
			return point(-the_world->field().length() * 0.5, 0);
		}

		/// Position of enemy goal.
		point goal_center() const {
			return point(the_world->field().length() * 0.5, 0);
		}

		/// Boundaries of own goal.
		std::pair<point, point> self_bounds() const {
			return std::make_pair(point(-the_world->field().length() * 0.5, -the_world->field().goal_width()),
					point(-the_world->field().length() * 0.5, the_world->field().goal_width()));
		}

		/// Boundaries of enemy goal.
		std::pair<point, point> goal_bounds() const {
			return std::make_pair(point(the_world->field().length() * 0.5, -the_world->field().goal_width()),
					point(the_world->field().length() * 0.5, the_world->field().goal_width()));
		}

		/// Clips a point to the outermost boundary.
		point clip_outermost(const point& p) const {
#warning TODO: implement
		}

		/**
		 * Checks if the passee can get the ball now.
		 * The passee must have line of sight to ball and facing the ball.
		 */
		bool has_ball_sight(const player::ptr passee) const;

		/// Number of points to consider when shooting at the goal.
		static const unsigned int SHOOTING_SAMPLE_POINTS;

		/// Instance of this world. No point hiding this.
		const world::ptr the_world;
}

#endif

