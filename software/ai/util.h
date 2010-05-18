#ifndef AI_UTIL_H
#define AI_UTIL_H

#include "ai/world/world.h"

#include <vector>

namespace ai_util {

	/**
	 * Orientation epsilon.
	 */
	extern const double ORI_CLOSE;

	/**
	 * Position epsilon.
	 */
	extern const double POS_CLOSE;

	/**
	 * Gets the orientation of a point.
	 */
	double orientation(const point& p);

	/**
	 * Gets the absolute angle difference.
	 * Guaranteed to be between 0 and PI.
	 */
	double angle_diff(const double& a, const double& b);

	/**
	 * Checks if the path from begin to end is blocked by one team, with some threshold.
	 * Returns true if path is okay.
	 */
	bool path_check(const point& begin, const point& end, const team& theteam, const double& thresh);

	/**
	 * Checks if the path from begin to end is blocked by one team, with some threshold.
	 * Also skips one particular robot.
	 * Returns true if path is okay.
	 */
	bool path_check(const point& begin, const point& end, const team& theteam, const double& thresh, const robot::ptr skip);

	/**
	 * Checks if the passee can get the ball now.
	 * Returns false if some robots is blocking line of sight of ball from passee
	 * Returns false if passee is not facing the ball.
	 * Returns false if some condition is invalid.
	 */
	bool can_pass(const world::ptr w, const player::ptr passee);

	/**
	 * Calculates the candidates to aim for when shooting at the goal.
	 */
	const std::vector<point> calc_candidates(const world::ptr w);

	/**
	 * Returns an integer i, where candidates[i] is the best point to aim for when shooting.
	 * Here candidates is the vector returned by calc_candidates.
	 * If all shots are bad, candidates.size() is returned.
	 */
	size_t calc_best_shot(const player::ptr player, const world::ptr w);

	/**
	 * Clips a point to a rectangle boundary.
	 */
	point clip_point(const point& p, const point& bound1, const point& bound2);
}

#endif

