#ifndef AI_HL_STP_EVALUATION_PLAYER_H
#define AI_HL_STP_EVALUATION_PLAYER_H

#include "ai/hl/stp/world.h"
#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Determines whether or not the player at position
				 * is facing within threshold degrees of the specified target
				 */
				bool player_within_angle_thresh(Player::CPtr player, const Point target, double threshold);

				/**
				 * Determines whether or not the player at position
				 * is facing within threshold degrees of the specified target
				 */
				bool player_within_angle_thresh(const Point position, const double orientation, const Point target, double threshold);
			}
		}
	}
}

#endif

