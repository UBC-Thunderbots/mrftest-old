#ifndef AI_HL_STP_EVALUATION_BALL_THREAT_H
#define AI_HL_STP_EVALUATION_BALL_THREAT_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				struct BallThreat {
					/**
					 * Enemy robot closest to ball.
					 */
					AI::HL::W::Robot::Ptr threat;

					/**
					 * Closest distance of enemy robot to ball.
					 */
					double threat_distance;

					/**
					 * Should steal mechanism be activated?
					 */
					bool activate_steal;

					/**
					 * Enemies sorted by distance to ball.
					 */
					std::vector<AI::HL::W::Robot::Ptr> enemies;
				};

				const BallThreat evaluate_ball_threat(const AI::HL::W::World& world);
			}
		}
	}
}

#endif

