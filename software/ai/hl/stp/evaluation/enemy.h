#ifndef AI_HL_STP_EVALUATION_ENEMY_H
#define AI_HL_STP_EVALUATION_ENEMY_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				struct EnemyThreat {

					/**
					 * min of dist of enemy robot to ball and our goal.
					 */
					double threat_dist;

					/**
					 * blocked by our player or not to our goal
					 */
					bool blocked;

					/**
					 * Other enemies that the enemy can pass to sorted by distance.
					 */
					std::vector<AI::HL::W::Robot::Ptr> pass_enemies;
				};
				
				/**
				 * Evaluate how dangerous an enemy is
				 */
				
				EnemyThreat eval_enemy(const AI::HL::W::World &world);
				
				
				
			}
		}
	}
}

#endif

