#ifndef AI_HL_STP_EVALUATION_SHOOT_H
#define AI_HL_STP_EVALUATION_SHOOT_H

#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/world.h"
#include "util/cacheable.h"
#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				using namespace AI::HL::STP::GradientApproach;

				struct ShootData final {
					bool blocked;
					bool reduced_radius;
					bool can_shoot;
					Angle angle;
					Angle accuracy_diff;
					Point target;
				};

				bool in_shoot_position(World world, Player player, Point target);
				ShootData evaluate_shoot(World world, Player player, bool use_reduced_radius = true);

				Point get_best_shot(World world, Robot robot);
				std::pair<Point, Angle> get_best_shot_pair(World world, Robot robot);

				double get_passee_shoot_score(const PassInfo::worldSnapshot& snap, Point position);

				/**
				 * The current score of the the robot, in it's current position and orientation
				 * a positive score indicates a scoring oppurtunity
				 * the return value represents the maximum angle error acceptable while still representing a goal scored
				 */
				Angle get_shoot_score(World world, Player player, bool use_reduced_radius = true);
				// double get_shoot_target(World world, Player player);

				// Point get_best_shoot_target(World world, Player player);

				// ShootData evaluate_shoot_target(World world, Player player, const Point target);

				/**
				 * Checks if a player can shoot to this location.
				 * A pass will ignore friendly robots for the purpose of obstacle avoidance.
				 */
				// bool can_shoot_target(World world, Player player, const Point target, bool pass = false);
			}
		}
	}
}

#endif

