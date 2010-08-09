#ifndef AI_UTIL_H
#define AI_UTIL_H

#include "ai/world/world.h"
#include "ai/world/team.h"
#include "uicomponents/param.h"

#include <vector>

/**
 * Contains a bunch of useful utility functions.
 * In general, functions that go here are those that
 * - can be used accross different roles/strategies/tactic.
 * - will unify definition (such as ball possesion).
 */
namespace AIUtil {

	/**
	 * A comparator that sorts by a particular distance.
	 * To be used together with std::sort.
	 * E.g.
	 * std::vector<Robot::Ptr> enemies = AIUtil::get_robots(enemy);
	 * std::sort(enemies.begin(), enemies.end(), AIUtil::CmpDist<Robot::Ptr>(goal));
	 */
	template<typename T> class CmpDist {
		public:
			CmpDist(const Point& dest) : dest(dest) {
			}
			bool operator()(T x, T y) const {
				return (x->position() - dest).lensq() < (y->position() - dest).lensq();
			}
		private:
			const Point& dest;
	};

	extern DoubleParam PLAYTYPE_WAIT_TIME;

	extern DoubleParam CHASE_BALL_DIST;

	extern DoubleParam DRIBBLE_TIMEOUT;

	/**
	 * If the robot orientation is within this angle,
	 * then it can shoot accurately.
	 */
	extern DoubleParam ORI_CLOSE;

	/**
	 * Somewhat close.
	 */
	static const double POS_CLOSE = Robot::MAX_RADIUS / 4.0;

	/**
	 * Really really really close.
	 * As in, we don't want division by zero.
	 */
	static const double POS_EPS = 1e-12;

	/**
	 * Somewhat stationary.
	 */
	static const double VEL_CLOSE = 1e-2;

	/**
	 * Super stationary.
	 */
	static const double VEL_EPS = 1e-12;

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
	bool ball_close(const World::Ptr w, const Robot::Ptr bot);

	/**
	 * Checks if a position is inside the friendly defense area
	 */
	bool point_in_defense(const World::Ptr w, const Point& pt);

	/**
	 * Checks if the path from begin to end is blocked by some robots.
	 * \param robots a vector of robots/players that blocks the path.
	 * \param thresh the amount of allowance for the path.
	 * For passing, use Robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE.
	 * For moving, use Robot::MAX_RADIUS * 2 + MOVE_ALLOWANCE.
	 * \return True if the path is not blocked.
	 */
	bool path_check(const Point& begin, const Point& end, const std::vector<Robot::Ptr>& robots, const double thresh);

	/**
	 * Checks if the path from begin to end is blocked by some obstacles.
	 * \param obstacles a vector of obstacles that blocks the path.
	 * \param thresh the amount of allowance for the path.
	 * For passing, use Robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE.
	 * For moving, use Robot::MAX_RADIUS * 2 + MOVE_ALLOWANCE.
	 * \return True if the path is not blocked.
	 */
	bool path_check(const Point& begin, const Point& end, const std::vector<Point>& obstacles, const double thresh);

	/**
	 * Checks if the passee can get the ball now.
	 * Returns false if some robots is blocking line of sight of ball from passee
	 * Returns false if passee is not facing the ball.
	 * Returns false if some condition is invalid.
	 */
	bool can_receive(const World::Ptr w, const Player::Ptr passee);

	/**
	 * Calculates the candidates to aim for when shooting at the goal.
	 */
	const std::vector<Point> calc_candidates(const World::Ptr w);

	/**
	 * Finds the length of the largest continuous interval (angle-wise)
	 * of the enemy goal that can be seen from a point.
	 * Having a vector of points enables one to add imaginary threats.
	 * Returns the point as well as the score.
	 * score is 0 if the point is invalid.
	 */
	std::pair<Point, double> calc_best_shot(const Field& f, const std::vector<Point>& obstacles, const Point& p, const double radius = Robot::MAX_RADIUS);
	std::pair<Point, double> calc_best_shot(const World::Ptr w, const Player::Ptr pl, const bool consider_friendly = true, const bool force_shoot = false);

	/**
	 * Returns the length of the largest continuous interval (angle-wise)
	 * of the enemy goal that can be seen from a point.
	 * Having a vector of points enables one to add imaginary threats.
	 * Returns 0 if the point is physically inside a considered robot.
	 */
	double calc_goal_visibility_angle(const World::Ptr w, const Player::Ptr pl, const bool consider_friendly = true);

	/**
	 * Convert friendly into vector of players, excluding some.
	 * Useful for separating robots in the Role and those which are not.
	 */
	std::vector<Player::Ptr> get_friends(const FriendlyTeam& friendly, const std::vector<Player::Ptr>& exclude);

	/**
	 * Finds the best player to pass to based on distance to the enemy goal.
	 * Returns -1 if no valid target is found.
	 */
	int choose_best_pass(const World::Ptr w, const std::vector<Player::Ptr>& friends);

	/**
	 * Returns true if the ball has the ball with high probability.
	 * Also uses vision.
	 */
	bool has_ball(const World::Ptr w, const Player::Ptr pl);

	/**
	 * Checks if a FRIENDLY PLAYER posses the ball.
	 */
	bool posses_ball(const World::Ptr w, const Player::Ptr p);

	/**
	 * Checks if an ENEMY ROBOT posses the ball.
	 */
	// bool posses_ball(const World::Ptr w, const Robot::Ptr r);

	/**
	 * Checks if the enemy team posses the ball.
	 */
	// bool enemy_posses_ball(const World::Ptr w);

	/**
	 * Checks if friendly team posses the ball.
	 */
	bool friendly_posses_ball(const World::Ptr w);
	bool friendly_has_ball(const World::Ptr w);

	/**
	 * Returns the player having the ball.
	 * If none has the ball, return -1.
	 */
	int calc_baller(const World::Ptr w, const std::vector<Player::Ptr>& players);
}

#endif

