#ifndef AI_HL_STP_ACTION_SHOOT_H
#define AI_HL_STP_ACTION_SHOOT_H

#include "ai/hl/stp/world.h"
#include "geom/rect.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Check whether or not the goalie should rush or not.
				 */
				bool goalie_rush(const World &world, Player::Ptr player, Point & rushpos);
				
				/**
				 * A single goalie and NO ONE ELSE defending the field.
				 */
				void lone_goalie(const World &world, Player::Ptr player);
				
				/**
				 * A goalie and NO ONE ELSE defending the field.
				 */
				void goalie_move(const World &world, Player::Ptr player, Point dest);
			}
		}
	}
}

#endif

