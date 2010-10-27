#ifndef AI_HL_UTIL_H
#define AI_HL_UTIL_H

#include "ai/hl/world.h"
#include "uicomponents/param.h"
#include <vector>

namespace AI {
	namespace HL {
		/**
		 * Contains a bunch of useful utility functions.
		 * In general, functions that go here are those that
		 * - can be used accross different roles/strategies/tactic.
		 * - will unify definition (such as ball possesion).
		 */
		namespace Util {
			/**
			 * A comparator that sorts by a particular distance.
			 * To be used together with std::sort.
			 * An object <var>x</var> is said to precede another object <var>y</var> is <var>x</var> is closer than <var>y</var> to the reference point.
			 *
			 * Example:
			 * <code>
			 * std::vector<AI::HL::W::Robot::Ptr> enemies = ai_util::get_robots(enemy);
			 * std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<AI::HL::W::Robot::Ptr>(goal));
			 * </code>
			 *
			 * \tparam T the type of object to sort (must have a function called \c position).
			 */
			template<typename T> class CmpDist {
				public:
					/**
					 * Constructs a new CmpDist.
					 *
					 * \param[in] dest the target point the distance to which to sort by.
					 */
					CmpDist(const Point &dest) : dest(dest) {
					}

					/**
					 * Compares two objects.
					 *
					 * \param[in] x the first object to compare.
					 *
					 * \param[in] y the second object to compare.
					 *
					 * \return \c true if \p x precedes \p y, or \c false if not.
					 */
					bool operator()(const T &x, const T &y) const {
						return (x->position() - dest).lensq() < (y->position() - dest).lensq();
					}

				private:
					const Point &dest;
			};

			/**
			 * General shooting accuracy in degrees.
			 */
			extern DoubleParam shoot_accuracy;

			/**
			 * Time the team can get ready for special plays.
			 * If the team spend too much time preparing for a freekick etc,
			 * the refree will force start.
			 * Reduce this number to prevent such occurence.
			 */
			extern DoubleParam get_ready_time;

			/**
			 * If dribble for more than this time, shoot already.
			 */
			extern DoubleParam dribble_timeout;

			/**
			 * Somewhat close.
			 */
			extern const double POS_CLOSE;

			/**
			 * Really really really close.
			 * As in, we don't want division by zero.
			 */
			extern const double POS_EPS;

			/**
			 * Somewhat stationary.
			 */
			extern const double VEL_CLOSE;

			/**
			 * Super stationary.
			 */
			extern const double VEL_EPS;

			/**
			 * If the robot is less than this angle away from the ball,
			 * then it is capable of receiving the ball.
			 */
			extern const double ORI_PASS_CLOSE;

			/**
			 * Let t be time elpased since robot has ball.
			 * If t < this number, then robot is considered to posses the ball.
			 */
			extern const double HAS_BALL_ALLOWANCE;

			/**
			 * Let t be time elpased since robot senses the ball.
			 * If t >= this number, then robot is considered to have the ball with very high probability.
			 */
			extern const double HAS_BALL_TIME;

			/**
			 * Checks if a point lies inside the friendly defense area.
			 * Useful for defenders.
			 */
			bool point_in_friendly_defense(AI::HL::W::World &world, const Point p);

			/**
			 * Checks if the path from begin to end is blocked by some obstacles.
			 *
			 * \param[in] obstacles a vector of obstacles that blocks the path.
			 *
			 * \param[in] thresh the amount of allowance for the path
			 * (for passing, use <code>Robot::MAX_RADIUS + Ball::RADIUS + SHOOT_ALLOWANCE</code>; for moving, use <code>Robot::MAX_RADIUS * 2 + MOVE_ALLOWANCE</code>).
			 *
			 * \return \c true if the path is not blocked, or \c false if it is blocked.
			 */
			bool path_check(const Point &begin, const Point &end, const std::vector<Point> &obstacles, double thresh);

			/**
			 * Checks if the path from begin to end is blocked by some robots.
			 *
			 * \param[in] robots a vector of robots/players that blocks the path.
			 *
			 * \param[in] thresh the amount of allowance for the path
			 * (for passing, use <code>Robot::MAX_RADIUS + Ball::RADIUS + SHOOT_ALLOWANCE</code>; for moving, use <code>Robot::MAX_RADIUS * 2 + MOVE_ALLOWANCE</code>).
			 *
			 * \return \c true if the path is not blocked, or \c false if it is.
			 */
			bool path_check(const Point &begin, const Point &end, const std::vector<AI::HL::W::Robot::Ptr> &robots, double thresh);

			/**
			 * Checks if the passee can get the ball now.
			 *
			 * \return \c false if some robots is blocking line of sight of ball from \p passee, if \p passee is not facing the ball, or if some condition is invalid;
			 * or \c true if \p passee can receive the ball.
			 */
			bool can_receive(AI::HL::W::World &world, AI::HL::W::Player::Ptr passee);

			/**
			 * Finds the length of the largest continuous interval (angle-wise) of the enemy goal that can be seen from a point.
			 * Having a vector of points enables one to add imaginary threats.
			 * This version accepts vector of obstacles, so that you can add imaginary robots.
			 *
			 * \param[in] f field is needed to calculate length etc
			 *
			 * \param[in] radius the multiplier to the radius of the robot,
			 * you can decrease the radius to make it easier to shoot.
			 *
			 * \return the point and the score (angle),
			 * where the score will be 0 if the point is invalid.
			 */
			std::pair<Point, double> calc_best_shot(const AI::HL::W::Field &f, const std::vector<Point> &obstacles, const Point &p, double radius = 1.0);

			/**
			 * Finds the length of the largest continuous interval (angle-wise) of the enemy goal that can be seen from a point.
			 * To add imaginary threats, please use the other version.
			 *
			 * \param[in] radius the multiplier to the radius of the robot,
			 * you can decrease the radius to make it easier to shoot.
			 *
			 * \return the point as and the score (angle),
			 * where the score will be 0 if the point is invalid,
			 */
			std::pair<Point, double> calc_best_shot(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, double radius = 1.0);

			/**
			 * Checks if the robot is in a position close enough to the ball to start
			 * So close that no other robot can be in the way of this ball.
			 */
			bool ball_close(AI::HL::W::World &world, AI::HL::W::Robot::Ptr robot);

			/**
			 * Checks if a FRIENDLY PLAYER posses the ball.
			 */
			bool posses_ball(AI::HL::W::World &world, AI::HL::W::Player::Ptr player);

			/**
			 * Checks if an ENEMY ROBOT posses the ball.
			 */
			bool posses_ball(AI::HL::W::World &world, AI::HL::W::Robot::Ptr robot);

			/**
			 * Finds the player having the ball.
			 *
			 * \return the player, or a null pointer if no friendly player has the ball.
			 */
			AI::HL::W::Player::Ptr calc_baller(AI::HL::W::World &world, const std::vector<AI::HL::W::Player::Ptr> &players);

			/**
			 * Converts a friendly team into a vector of players.
			 */
			std::vector<AI::HL::W::Player::Ptr> get_players(AI::HL::W::FriendlyTeam &friendly);

			/**
			 * Converts an enemy team into a vector of robots.
			 */
			std::vector<AI::HL::W::Robot::Ptr> get_robots(AI::HL::W::EnemyTeam &enemy);

			/**
			 * Reorders a vector of waypoints to match a vector of players.
			 */
			void waypoints_matching(const std::vector<AI::HL::W::Player::Ptr>& players, std::vector<Point>& waypoints);
		}
	}
}

#endif

