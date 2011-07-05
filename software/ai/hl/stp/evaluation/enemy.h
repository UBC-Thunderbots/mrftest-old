#ifndef AI_HL_STP_EVALUATION_ENEMY_H
#define AI_HL_STP_EVALUATION_ENEMY_H

#include "ai/hl/stp/world.h"

#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * How dangerous an enemy is
				 */
				struct Threat {
					/**
					 * How much it can see the goal.
					 */
					double goal_angle;

					/**
					 * Is this robot capable of shooting the target?
					 */
					bool can_shoot_goal;

					/**
					 * How many passes for the BALL to REACH this player.
					 * 0 - posses the ball
					 * 1 - need one pass
					 * 5 - dont bother
					 */
					int passes_reach;

					/**
					 * How many passes to SHOOT on net,
					 * if this player HAS the BALL.
					 * 5 - impossible
					 */
					int passes_goal;

					/**
					 * For convenience.
					 * A pointer to the robot.
					 */
					Robot::Ptr robot;

					/**
					 * If this robot needs to shoot at someone before shooting goal.
					 */
					Robot::Ptr passee;

					/**
					 * If this robot needs to receive the ball from someone.
					 */
					Robot::Ptr passer;
				};

				/**
				 * Evaluates each robot and calculates how dangerous they are.
				 */
				std::vector<Threat> calc_enemy_threat(const World& world);

				/**
				 * Checks if it's possible for this enemy to shoot to the goal.
				 */
				bool enemy_can_shoot_goal(const World& world, const Robot::Ptr enemy);

				/**
				 * Calculates how good an enemy is at shooting our goal.
				 */
				std::pair<Point, double> calc_enemy_best_shot_goal(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius = 1.0);

				/**
				 * Calculates how good an enemy is at shooting our goal.
				 */
				std::pair<Point, double> calc_enemy_best_shot_goal(const World &world, const Robot::Ptr enemy, const double radius = 1.0);

				/**
				 * # of passes it takes for the enemy to shoot to our goal
				 * 0 means the enemy has a clear shot to our goal!
				 * ignore (set to 5 if # of passes > 2)
				 */
				int calc_enemy_pass(const World &world, Robot::Ptr robot);

				/**
				 * BAD STUFF
				 */
				std::vector<Robot::Ptr> get_passees(const World& world, Robot::Ptr robot);

				/**
				 * Given obstacle position, calculates the min amount of passing
				 */
				std::vector<int> calc_min_enemy_pass(const std::vector<Point> obstacles, const std::vector<Point> enemies);
			}
		}
	}
}

#endif

