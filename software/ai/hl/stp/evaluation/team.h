#ifndef AI_HL_STP_EVALUATION_TEAM_H
#define AI_HL_STP_EVALUATION_TEAM_H

#include "ai/hl/stp/world.h"
#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Obtains the nearest friendly player.
				 */
				Player::CPtr nearest_friendly(const World &world, Point target);

				/**
				 * Obtains the nearest enemy robot.
				 */
				Robot::Ptr nearest_enemy(const World &world, Point target);
			}
		}
	}
}

#endif

