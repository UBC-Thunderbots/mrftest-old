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
					 * blocked by our players or other enemies or not to our goal
					 */
					bool blocked;
					
					/**
					 * Other enemies that the enemy can pass to sorted by distance.
					 */
					std::vector<AI::HL::W::Robot::Ptr> pass_enemies;
									
					/**
					 * # of passes it takes to shoot to our goal
					 * ignore (set to 5 if # of passes > 2)
					 */
					int passes;
				};
				
				/**
				 * Evaluate how dangerous an enemy is
				 */
				
				EnemyThreat eval_enemy(const AI::HL::W::World &world, AI::HL::W::Robot::Ptr robot);
								
			}
		}
	}
}

#endif

