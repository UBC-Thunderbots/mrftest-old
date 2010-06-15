#ifndef AI_UTIL_H
#define AI_UTIL_H

#include "ai/world/world.h"
#include "ai/world/team.h"

#include <vector>

/**
 * Contains a bunch of useful utility functions.
 * In general, functions that go here are those that
 * - can be used accross different roles/strategies/tactic.
 * - will unify definition (such as ball possesion).
 */
namespace ai_util {

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

	// TODO: base this on distance.
	/**
	 * If the robot orientation is within this angle,
	 * then it can shoot accurately.
	 */
	static const double ORI_CLOSE = 5.0 / 180.0 * M_PI;

	/**
	 * Somewhat close.
	 * As in, the ball touching the robot.
	 */
	static const double POS_CLOSE = robot::MAX_RADIUS / 2.0;

	/**
	 * Really really really close.
	 */
	static const double POS_EPS = 1e-10;

	/**
	 * Velocity epsilon.
	 */
	static const double VEL_CLOSE = 1e-2;

	/**
	 * If the robot is less than this angle away from the ball,
	 * then it is capable of receiving the ball.
	 */
	static const double ORI_PASS_CLOSE = 45.0 / 180.0 * M_PI;

	/**
	 * Let t be time elpased since robot has ball.
	 * If t < this number, then robot is considered to posses the ball.
	 */
	static const double HAS_BALL_ALLOWANCE = 3.0;

	/**
	 * Let t be time elpased since robot senses the ball.
	 * If t >= this number, then robot is considered to have the ball with very high probability.
	 */
	static const double HAS_BALL_TIME = 2.0 / 15.0;

	/**
	 * Number of points to consider when shooting at the goal.
	 */
	// static const unsigned int SHOOTING_SAMPLE_POINTS = 11;

	/**
	 * Checks if the robot is in a position close enough to the ball to start
	 * So close that no other robot can be in the way of this ball.
	 */
	bool ball_close(const world::ptr w, const player::ptr bot);

	/**
	 * Checks if a position is inside the friendly defense area
	 */
	bool point_in_defense(const world::ptr w, const point& pt);

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

	// TODO: maybe the source to a point instead of defaulting to ball.
	/**
	 * Checks if the passee can get the ball now.
	 * Returns false if some robots is blocking line of sight of ball from passee
	 * Returns false if passee is not facing the ball.
	 * Returns false if some condition is invalid.
	 */
	bool can_receive(const world::ptr w, const player::ptr passee);

	/**
	 * Calculates the candidates to aim for when shooting at the goal.
	 */
	const std::vector<point> calc_candidates(const world::ptr w);

	/**
	 * Finds the length of the largest continuous interval (angle-wise)
	 * of the enemy goal that can be seen from a point.
	 * Having a vector of points enables one to add imaginary threats.
	 * Returns the point as well as the score.
	 * score is 0 if the point is invalid.
	 */
	std::pair<point, double> calc_best_shot(const field& f, const std::vector<point>& obstacles, const point& p);
	std::pair<point, double> calc_best_shot(const world::ptr w, const player::ptr pl, const bool consider_friendly = true);

	/**
	 * Returns the length of the largest continuous interval (angle-wise)
	 * of the enemy goal that can be seen from a point.
	 * Having a vector of points enables one to add imaginary threats.
	 * Returns 0 if the point is physically inside a considered robot.
	 */
	double calc_goal_visibility_angle(const world::ptr w, const player::ptr pl, const bool consider_friendly = true);

	/**
	 * Convert friendly into vector of players, excluding some.
	 * Useful for separating robots in the role and those which are not.
	 */
	std::vector<player::ptr> get_friends(const friendly_team& friendly, const std::vector<player::ptr>& exclude);

	/**
	 * Finds the best player to pass to based on distance to the enemy goal.
	 * Returns -1 if no valid target is found.
	 */
	int choose_best_pass(const world::ptr w, const std::vector<player::ptr>& friends);

	/**
	 * Returns true if the ball has the ball with high probability.
	 * Also depends on AI.
	 */
	bool has_ball(const player::ptr pl);

	/**
	 * Returns true if the ball has the ball with high probability.
	 * Also depends on AI.
	 */
	bool has_ball(const world::ptr w, const player::ptr pl);

	/**
	 * Checks if a player posses the ball.
	 * Useful for strategy to know if team should go offence or defence.
	 * This function is based on whether the player has ball, or recently has the ball.
	 */
	bool posses_ball(const world::ptr w, const player::ptr p);
}

#endif

