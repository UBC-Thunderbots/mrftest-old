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
					 * blocked by our players or other enemies for passing
					 */
					bool blocked;
					
					/**
					 * Other enemies that the enemy can pass to sorted by distance.
					 */
					std::vector<AI::HL::W::Robot::Ptr> passees;
									
					/**
					 * # of passes it takes for the enemy to shoot to our goal
					 * 0 means the enemy has a clear shot to our goal!
					 * ignore (set to 5 if # of passes > 2)
					 */
					int passes;
				};
				
				/**
				 * Evaluate how dangerous an enemy is
				 */
				
				EnemyThreat eval_enemy(const AI::HL::W::World &world, const AI::HL::W::Robot::Ptr robot);
								
			}
		}
	}
}

#endif

