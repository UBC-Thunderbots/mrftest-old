#ifndef AI_HL_STP_ACTION_SHOOT_H
#define AI_HL_STP_ACTION_SHOOT_H

#include "ai/hl/stp/world.h"
#include "geom/rect.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * A single goalie and NO ONE ELSE defending the field.
				 */
				void lone_goalie(const World &world, Player::Ptr player);
				
				/**
				 * Move the goalie to this location.
				 * If the ball is dangerously moving towards the net,
				 * then rush to defend it.
				 */
				void goalie_move(const World &world, Player::Ptr player, Point dest);
				
				/**
				 * Penalty Goalie
				 */
				void penalty_goalie(const World &world, Player::Ptr player);
			}
		}
	}
}

#endif

