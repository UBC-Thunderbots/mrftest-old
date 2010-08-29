#ifndef AI_UTIL_H
#define AI_UTIL_H

#include "ai/world/world.h"
#include "ai/world/team.h"
#include "uicomponents/param.h"

#include <vector>

/**
 * Contains a bunch of useful utility functions.
 * In general, the functions that go here:
 * - do geometric calculations, without containing any form of INTELLIGENCE.
 * - can be used accross different roles/strategies/tactic.
 * - will unify definition (such as ball possesion).
 */
namespace AI {
	namespace Util {
	/**
	 * A comparator that sorts by a particular distance.
	 *
	 * Example:
	 * \code
	 * std::vector<Robot::Ptr> enemies = AIUtil::get_robots(enemy);
	 * std::sort(enemies.begin(), enemies.end(), AIUtil::CmpDist<Robot::Ptr>(goal));
	 * \endcode
	 *
	 * \tparam T the type of object whose distances should be compared.
	 */
	template<typename T> class CmpDist {
		public:
			/**
			 * Creates a comparator.
			 *
			 * \param[in] dest the point by distances to which the objects
			 * should be compared.
			 */
			CmpDist(const Point& dest) : dest(dest) {
			}

			/**
			 * Compares two objects.
			 *
			 * \param[in] x the first object.
			 *
			 * \param[in] y the second object.
			 *
			 * \return \c true if \p x is closer to the destination than \c y,
			 * or \c false if not.
			 */
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
	 * Checks if the robot is in a position close enough to the ball to start
	 * So close that no other robot can be in the way of this ball.
	 *
	 * \param[in] w the World in which to perform the check.
	 *
	 * \param[in] bot the Robot to check.
	 */
	bool ball_close(World &w, const Robot::Ptr bot);

	/**
	 * Checks if a position is inside the friendly defense area.
	 *
	 * \param[in] w the World in which to perform the check.
	 *
	 * \param[in] pt the Point to check.
	 */
	bool point_in_defense(World &w, const Point& pt);

	/**
	 * Checks if the path from \p begin to \p end is blocked by some robots.
	 *
	 * \param[in] begin the beginning of the path.
	 *
	 * \param[in] end the end of the path.
	 *
	 * \param[in] robots a vector of robots/players that blocks the path.
	 *
	 * \param[in] thresh the amount of allowance for the path (for passing, use
	 * <code>Robot::MAX_RADIUS + Ball::RADIUS + AIUtil::SHOOT_ALLOWANCE</code>;
	 * for moving, use <code>Robot::MAX_RADIUS * 2 +
	 * AIUtil::MOVE_ALLOWANCE</code>).
	 *
	 * \return \c true if the path is not blocked.
	 */
	bool path_check(const Point& begin, const Point& end, const std::vector<Robot::Ptr>& robots, const double thresh);

	/**
	 * Checks if the path from \p begin to \p end is blocked by some obstacles.
	 *
	 * \param[in] begin the beginning of the path.
	 *
	 * \param[in] end the end of the path.
	 *
	 * \param[in] obstacles a vector of obstacles that blocks the path.
	 *
	 * \param[in] thresh the amount of allowance for the path (for passing, use
	 * <code>Robot::MAX_RADIUS + Ball::RADIUS + AIUtil::SHOOT_ALLOWANCE</code>;
	 * for moving, use <code>Robot::MAX_RADIUS * 2 +
	 * AIUtil::MOVE_ALLOWANCE</code>).
	 *
	 * \return \c true if the path is not blocked.
	 */
	bool path_check(const Point& begin, const Point& end, const std::vector<Point>& obstacles, const double thresh);

	/**
	 * Checks if the passee can get the ball now.
	 *
	 * \param[in] w the World in which to check.
	 *
	 * \param[in] passee the Player trying to receive the ball.
	 *
	 * \return \c false if some robots is blocking line of sight of ball from
	 * \p passee, if \p passee is not facing the ball, or if some condition is
	 * invalid, or \c true otherwise.
	 */
	bool can_receive(World &w, const Player::Ptr passee);

	/**
	 * Calculates the candidates to aim for when shooting at the goal.
	 *
	 * \param[in] w the World in which to calculate.
	 *
	 * \return a collection of open points in the goal.
	 */
	const std::vector<Point> calc_candidates(World &w);

	/**
	 * Calculates the best possible point to shoot the ball.
	 * Specifially, finds the length of the largest continuous interval (angle-wise) of the enemy goal that can be seen from a point.
	 * Having a vector of points enables one to add imaginary threats.
	 *
	 * \param[in] f the Field in which to search.
	 *
	 * \param[in] obstacles the obstacles that potentially block the goal.
	 *
	 * \param[in] p the origin point from which to look.
	 *
	 * \param[in] radius the radius of the obstacles.
	 * By default, uses robot radius.
	 *
	 * \return a pair \c (p, s) where \c p is the middle of the best interval
	 * and \c s is the angle of that interval, where a score of 0 means the enemy goal cannot be seen.
	 */
	std::pair<Point, double> calc_best_shot(const Field& f, const std::vector<Point>& obstacles, const Point& p, const double radius = Robot::MAX_RADIUS);

	/**
	 * Calculates the best possible point to shoot the ball.
	 * Specifially, finds the length of the largest continuous interval (angle-wise) of the enemy goal that can be seen from a point.
	 *
	 * \param[in] w the World in which to search.
	 *
	 * \param[in] pl the Player from whose position to look.
	 *
	 * \param[in] consider_friendly treat friendly players as obstacles.
	 *
	 * \return a pair \c (p, s) where \c p is the middle of the best interval
	 * and \c s is the angle of that interval, where a score of 0 means the enemy goal cannot be seen.
	 */
	std::pair<Point, double> calc_best_shot(const World &w, const Player::Ptr pl, const bool consider_friendly = true);

	/**
	 * Returns the length of the largest continuous interval (angle-wise) of the
	 * enemy goal that can be seen from a point. Having a vector of points
	 * enables one to add imaginary threats.
	 *
	 * \param[in] w the World in which to search.
	 *
	 * \param[in] pl the Player from whose position to look.
	 *
	 * \param[in] consider_friendly treat friendly players as obstacles.
	 *
	 * \return the angle of the interval, and 0 if the enemy goal cannot be seen.
	 */
	double calc_goal_visibility_angle(const World &w, const Player::Ptr pl, const bool consider_friendly = true);

	/**
	 * Converts \p friendly into a \c vector of \ref Player "Players", excluding
	 * some of them. Useful for separating robots in the Role and those which
	 * are not.
	 *
	 * \param[in] friendly the team whose \ref Player "Players" to return.
	 *
	 * \param[in] exclude the \ref Player "Players" to exclude.
	 *
	 * \return all the \ref Player "Players" in \p friendly except for those in
	 * \p exclude.
	 */
	std::vector<Player::Ptr> get_friends(const FriendlyTeam& friendly, const std::vector<Player::Ptr>& exclude);

	/**
	 * Finds the best player to pass to based on distance to the enemy goal.
	 *
	 * \param[in] w the World in which to search.
	 *
	 * \param[in] friends the \ref Player "Players" to consider.
	 *
	 * \return ?, or -1 if no valid target is found.
	 */
	int choose_best_pass(World &w, const std::vector<Player::Ptr>& friends);

	/**
	 * Checks whether a player probably has the ball. Uses dribbler sensing and
	 * also, if enabled in the parameters table, vision.
	 *
	 * \param[in] w the World in which to check.
	 *
	 * \param[in] pl the Player to consider.
	 *
	 * \return \c true if \p pl has the ball with high probability.
	 */
	bool has_ball(World &w, const Player::Ptr pl);

	/**
	 * Checks if a FRIENDLY PLAYER posses the ball. Possession is defined as
	 * either having the ball or, if enabled in the parameters table, also the
	 * ball being close to the player.
	 *
	 * \param[in] w the World in which to check.
	 *
	 * \param[in] p the Player to consider.
	 *
	 * \return \c true if \p p possesses the ball.
	 */
	bool posses_ball(World &w, const Player::Ptr p);

	/**
	 * Checks if an ENEMY ROBOT posses the ball.
	 */
	// bool posses_ball(World &w, const Robot::Ptr r);

	/**
	 * Checks if the enemy team posses the ball.
	 */
	// bool enemy_posses_ball(World &w);

	/**
	 * Checks if the friendly team possesses the ball.
	 *
	 * \param[in] w the World in which to check.
	 *
	 * \return \c true if the friendly team possesses the ball.
	 */
	bool friendly_posses_ball(World &w);

	/**
	 * Checks if the friendly team has the ball.
	 *
	 * \param[in] w the World in which to check.
	 *
	 * \return \c true if the friendly team has the ball.
	 */
	bool friendly_has_ball(World &w);

	/**
	 * Determines which player has the ball.
	 *
	 * \param[in] w the World in which to check.
	 *
	 * \param[in] players the \ref Player "Players" to examine.
	 *
	 * \return the index in \p players of the Player that has the ball, or -1 if
	 * none have the ball.
	 */
	int calc_baller(World &w, const std::vector<Player::Ptr>& players);
}
}

#endif

