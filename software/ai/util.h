#ifndef AI_UTIL_H
#define AI_UTIL_H

#include "ai/world/world.h"
#include "ai/world/team.h"

#include <vector>

/**
 * Contains a bunch of useful utility functions.
 */
namespace ai_util {

	/**
	 * A comparator that sorts by values in a vector
	 */
	template<typename T> class cmp_table {
		public:
			cmp_table(const std::vector<T>& tbl) : tbl(tbl) {
			}
			bool operator()(unsigned int x, unsigned int y) {
				return tbl[x] > tbl[y];
			}
		private:
			const std::vector<T>& tbl;
	};

	/**
	 * A comparator that sorts by a particular distance.
	 * To be used together with std::sort.
	 * E.g.
	 * std::vector<robot::ptr> enemies = ai_util::get_robots(enemy);
	 * std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<robot::ptr>(goal));
	 */
	template<typename T> class cmp_dist {
		public:
			cmp_dist(const point& dest) : dest(dest) {
			}
			bool operator()(T x, T y) const {
				return (x->position() - dest).lensq() < (y->position() - dest).lensq();
			}
		private:
			const point& dest;
	};

	/**
	 * Orientation epsilon.
	 * Generally higher than position epsilon.
	 */
	extern const double ORI_CLOSE;

	/**
	 * Position epsilon.
	 * Should be set to the accuracy of the image recognizition data.
	 */
	extern const double POS_CLOSE;

	/**
	 * Velocity epsilon.
	 */
	extern const double VEL_CLOSE;

	/**
	 * Number of points to consider when shooting at the goal.
	 */
	extern const unsigned int SHOOTING_SAMPLE_POINTS;


	/**
	*Checks if the robot is in a position close enough to the ball to start
	*the dribbler motor would be nice to check for obstacles in the way of ball before doing this
	*/
	bool ball_close(const world::ptr w, const player::ptr bot);
	
	/**
	 * Checks if the path from begin to end is blocked by some robots.
	 * \param robots a vector of robots/players that blocks the path.
	 * \param thresh the amount of allowance for the path.
	 * For passing, use robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE.
	 * For moving, use robot::MAX_RADIUS * 2 + MOVE_ALLOWANCE.
	 * \return True if the path is not blocked.
	 */
	bool path_check(const point& begin, const point& end, const std::vector<robot::ptr>& robots, const double thresh);
	
	/**
	 * Checks if the path from begin to end is blocked by some obstacles.
	 * \param obstacles a vector of obstacles that blocks the path.
	 * \param thresh the amount of allowance for the path.
	 * For passing, use robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE.
	 * For moving, use robot::MAX_RADIUS * 2 + MOVE_ALLOWANCE.
	 * \return True if the path is not blocked.
	 */
	bool path_check(const point& begin, const point& end, const std::vector<point>& obstacles, const double thresh);

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
	 * If all shots are bad, -1 is returned.
	 */
	int calc_best_shot(const player::ptr player, const world::ptr w);

	/**
	 * Convert friendly into vector of players, excluding some.
	 * Useful for separating robots in the role and those which are not.
	 */
	std::vector<player::ptr> get_friends(const friendly_team& friendly, const std::vector<player::ptr>& exclude);

	/**
	 * Matches points of two different vectors.
	 * Returns ordering of the matching such that the total distance is minimized.
	 * If order is the returned vector.
	 * Then the i element of v1 is matched with order[i] element of v2.
	 * Currently uses a slow brute-force algorithm.
	 */
	std::vector<size_t> dist_matching(const std::vector<point>& v1, const std::vector<point>& v2);

	/**
	 * Finds the best player to pass to based on distance to the enemy goal.
	 * Returns -1 if no valid target is found.
	 */
	int choose_best_pass(const world::ptr w, const std::vector<player::ptr>& friends);
	
	/**
	 * Returns the length of the largest continuous interval (angle-wise)
	 * of the enemy goal that can be seen from a point.
	 * Having a vector of points enables one to add imaginary threats.
	 * Returns 0 if the point is physically inside a considered robot.
	 */
	double calc_goal_visibility_angle(const field& f, const std::vector<point>& robots, const point& p);
}

#endif

