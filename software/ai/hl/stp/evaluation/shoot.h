#ifndef AI_HL_STP_EVALUATION_SHOOT_H
#define AI_HL_STP_EVALUATION_SHOOT_H

#include "ai/hl/stp/world.h"
#include "util/cacheable.h"
#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				struct ShootData {
					bool blocked;
					bool reduced_radius;
					bool can_shoot;
					double angle;
					double accuracy_diff;
					Point target;
				};

				ShootData evaluate_shoot(const World &world, Player::CPtr player);

				ShootData evaluate_shoot_target(const World &world, Player::CPtr player, const Point target);
				
				/**
				 * Checks if a player can shoot to this location.
				 * A pass will ignore friendly robots for the purpose of obstacle avoidance.
				 */
				bool can_shoot_target(const World &world, Player::CPtr player, const Point target, bool pass = false);
			}
		}
	}
}

#endif

