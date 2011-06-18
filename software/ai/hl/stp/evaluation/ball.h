#ifndef AI_HL_STP_EVALUATION_BALL_H
#define AI_HL_STP_EVALUATION_BALL_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * The ball is right in front of the player.
				 */
				bool possess_ball(const World &world, Player::CPtr player);
			}
		}
	}
}

#endif

