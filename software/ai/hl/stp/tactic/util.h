#ifndef AI_HL_STP_TACTIC_UTIL_H
#define AI_HL_STP_TACTIC_UTIL_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Selects a player that has the ball if possible.
				 * Otherwise, picks a player closest to the ball.
				 */
				Player::Ptr select_baller(const World& world, const std::set<Player::Ptr> &players);
			}
		}
	}
}

#endif

