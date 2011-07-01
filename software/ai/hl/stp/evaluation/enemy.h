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
					bool can_shoot;

					/**
					 * How many passes to reach the player.
					 * 0 - posses the ball
					 * 1 - need one pass
					 */
					int passes;

					/**
					 * For convenience.
					 * A pointer to the robot.
					 */
					Robot::Ptr robot;
				};

				/**
				 * Evaluates each robot and calculates how dangerous they are.
				 */
				std::vector<Threat> calc_enemy_threat(const World& world);

				/**
				 * Calculates how good an enemy is at shooting our goal.
				 */
				std::pair<Point, double> calc_enemy_best_shot(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius);

				/**
				 * Calculates how good an enemy is at shooting our goal.
				 */
				std::pair<Point, double> calc_enemy_best_shot(const World &world, const Robot::Ptr enemy, const double radius = 1.0);

				/**
				 * # of passes it takes for the enemy to shoot to our goal
				 * 0 means the enemy has a clear shot to our goal!
				 * ignore (set to 5 if # of passes > 2)
				 */
				int calc_enemy_pass(const World &world, Robot::Ptr robot);

				std::vector<Robot::Ptr> get_passees(const World& world, Robot::Ptr robot);
			}
		}
	}
}

#endif

