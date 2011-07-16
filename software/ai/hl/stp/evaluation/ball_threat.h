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

				BallThreat evaluate_ball_threat(const AI::HL::W::World &world);

				/**
				 * Assesses whether a ball is heading towards our net
				 */
				bool ball_on_net(const AI::HL::W::World &world);

				/**
				 * Assesses whether a ball is heading towards their net
				 */
				bool ball_on_enemy_net(const AI::HL::W::World &world);

				/**
				 * Get where the goalie should go to block the shot
				 */
				Point goalie_shot_block(const AI::HL::W::World &world, AI::HL::W::Player::Ptr player);
			}
		}
	}
}

#endif

