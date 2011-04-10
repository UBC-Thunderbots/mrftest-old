#ifndef AI_HL_UTIL_H
#define AI_HL_UTIL_H

#include "ai/hl/world.h"
#include "util/param.h"
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
			bool point_in_friendly_defense(const AI::HL::W::Field &field, const Point p);

			/**
			 * If the point is outside the field boundary it is cropped to within the field boundaries.
			 */
			Point crop_point_to_field(const AI::HL::W::Field &field, const Point p);

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
			std::pair<Point, double> calc_best_shot(const AI::HL::W::World &world, AI::HL::W::Player::CPtr player, double radius = 1.0);

			/**
			 * Finds the length of the largest continuous interval (angle-wise) of a target player (passee) that can be seen from a point.
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
			std::pair<Point, double> calc_best_shot_target(const Point &target_pos, const std::vector<Point> &obstacles, const Point &p, double radius = 1.0);

			/**
			 * Finds the length of the largest continuous interval (angle-wise) of a target player (passee) that can be seen from a point.
			 * To add imaginary threats, please use the other version.
			 *
			 * \param[in] radius the multiplier to the radius of the robot,
			 * you can decrease the radius to make it easier to shoot.
			 *
			 * \return the point as and the score (angle),
			 * where the score will be 0 if the point is invalid,
			 */
			std::pair<Point, double> calc_best_shot_target(const AI::HL::W::World &world, const Point &target_pos, AI::HL::W::Player::CPtr player, double radius = 1.0);


			/**
			 * Checks if the robot is in a position close enough to the ball to start
			 * So close that no other robot can be in the way of this ball.
			 */
			bool ball_close(const AI::HL::W::World &world, AI::HL::W::Robot::Ptr robot);

			/**
			 * Checks if a FRIENDLY PLAYER posses the ball.
			 */
			bool posses_ball(const AI::HL::W::World &world, AI::HL::W::Player::Ptr player);

			/**
			 * Checks if an ENEMY ROBOT posses the ball.
			 */
			bool posses_ball(const AI::HL::W::World &world, AI::HL::W::Robot::Ptr robot);

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

			std::vector<AI::HL::W::Player::CPtr> get_players(const AI::HL::W::FriendlyTeam &friendly);

			/**
			 * Converts a friendly team into a vector of players.
			 */
			std::vector<AI::HL::W::Player::Ptr> get_players_exclude(AI::HL::W::FriendlyTeam &friendly, std::vector<AI::HL::W::Player::Ptr> &others);

			/**
			 * Converts an enemy team into a vector of robots.
			 */
			std::vector<AI::HL::W::Robot::Ptr> get_robots(const AI::HL::W::EnemyTeam &enemy);

			/**
			 * Finds the best assignment of players to points.
			 * This is done by computing a minimum-total-distance bipartite matching between the sets of points.
			 * This function should work even if the number of players and waypoints do not match.
			 *
			 * \param[in] players a list of players.
			 *
			 * \param[in, out] waypoints a list of points in which to assign the players, which will be reordered.
			 */
			void waypoints_matching(const std::vector<AI::HL::W::Player::Ptr> &players, std::vector<Point> &waypoints);

			/**
			 * Finds the best player to pass to.
			 * This player must:
			 * - see a significant portion of the enemy goal
			 * - able to receive the ball
			 *
			 * \param[in] friends the list of players to pass to.
			 */
			AI::HL::W::Player::Ptr choose_best_pass(AI::HL::W::World &world, const std::vector<AI::HL::W::Player::Ptr> &friends);

			/**
			 * TODO
			 * Computes a scoring function on how good it is to pass the ball to a particular player.
			 * This is based on combination of:
			 * - angle that the ball can see the player
			 * - amount of blockage enemy has to the ball.
			 *
			 * This score assumes the ball travels at infinite velocity,
			 * but the players have known accuracy.
			 *
			 * \param[in] passee the player to pass to.
			 *
			 */
			double calc_pass_score(const AI::HL::W::World &world, const AI::HL::W::Player::Ptr passee);
		}
	}
}

#endif

