#ifndef AI_HL_UTIL_H
#define AI_HL_UTIL_H

#include "ai/hl/world.h"
#include "uicomponents/param.h"

#include <vector>

/**
 * Contains a bunch of useful utility functions.
 * In general, functions that go here are those that
 * - can be used accross different roles/strategies/tactic.
 * - will unify definition (such as ball possesion).
 */
namespace AI {
	namespace HL {
		namespace Util {

			/**
			 * A comparator that sorts by a particular distance.
			 * To be used together with std::sort.
			 * E.g.
			 * std::vector<AI::HL::W::Robot::Ptr> enemies = ai_util::get_robots(enemy);
			 * std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<AI::HL::W::Robot::Ptr>(goal));
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
			 * TODO: base this on distance.
			 */
			extern DoubleParam ORI_CLOSE;

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
			 * Checks if the path from begin to end is blocked by some obstacles.
			 * \param obstacles a vector of obstacles that blocks the path.
			 * \param thresh the amount of allowance for the path.
			 * For passing, use robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE.
			 * For moving, use robot::MAX_RADIUS * 2 + MOVE_ALLOWANCE.
			 * \return True if the path is not blocked.
			 */
			bool path_check(const Point& begin, const Point& end, const std::vector<Point>& obstacles, const double thresh);

			/**
			 * Checks if the path from begin to end is blocked by some robots.
			 * \param robots a vector of robots/players that blocks the path.
			 * \param thresh the amount of allowance for the path.
			 * For passing, use robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE.
			 * For moving, use robot::MAX_RADIUS * 2 + MOVE_ALLOWANCE.
			 * \return True if the path is not blocked.
			 * TODO: add more features to this function
			 */
			bool path_check(const Point& begin, const Point& end, const std::vector<AI::HL::W::Robot::Ptr>& robots, const double thresh);

			/**
			 * Checks if the passee can get the ball now.
			 * Returns false if some robots is blocking line of sight of ball from passee
			 * Returns false if passee is not facing the ball.
			 * Returns false if some condition is invalid.
			 * TODO: maybe the source to a point instead of defaulting to ball.
			 */
			bool can_receive(AI::HL::W::World& world, const AI::HL::W::Player::Ptr passee);

			/**
			 * Finds the length of the largest continuous interval (angle-wise)
			 * of the enemy goal that can be seen from a point.
			 * Having a vector of points enables one to add imaginary threats.
			 * Returns the point as well as the score.
			 * score is 0 if the point is invalid.
			 * This version accepts vector of obstacles, so that you can add imaginary robots.
			 * \param[in] f field is needed to calculate length etc
			 * \param[in] radius the radius of the robot, you can decrease the radius to make it easier to shoot.
			 */
			std::pair<Point, double> calc_best_shot(const AI::HL::W::Field& f, const std::vector<Point>& obstacles, const Point& p, const double radius = AI::HL::W::Robot::MAX_RADIUS);

			/**
			 * Finds the length of the largest continuous interval (angle-wise)
			 * of the enemy goal that can be seen from a point.
			 * Having a vector of points enables one to add imaginary threats.
			 * Returns the point as well as the score.
			 * score is 0 if the point is invalid.
			 */
			std::pair<Point, double> calc_best_shot(AI::HL::W::World &world, const AI::HL::W::Player::Ptr p);

			/**
			 * Checks if the robot is in a position close enough to the ball to start
			 * So close that no other robot can be in the way of this ball.
			 */
			bool ball_close(AI::HL::W::World &world, const AI::HL::W::Robot::Ptr robot);

			/**
			 * Checks if a FRIENDLY PLAYER posses the ball.
			 */
			bool posses_ball(AI::HL::W::World& world, const AI::HL::W::Player::Ptr player);

			/**
			 * Checks if an ENEMY ROBOT posses the ball.
			 */
			bool posses_ball(AI::HL::W::World& world, const AI::HL::W::Robot::Ptr robot);

			/**
			 * Finds the player having the ball.
			 * Returns NULL, if no such player exist.
			 */
			AI::HL::W::Player::Ptr calc_baller(AI::HL::W::World &world, const std::vector<AI::HL::W::Player::Ptr>& players);
		}
	}
}

#endif

