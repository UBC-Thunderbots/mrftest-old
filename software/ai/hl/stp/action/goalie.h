#pragma once
#include "ai/hl/stp/world.h"
#include "geom/rect.h"
#include "ai/hl/stp/action/action.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Loan Goalie
				 *
				 * Intended for goalie use
				 *
				 * A single goalie and NO ONE ELSE defending the field.
				 */
				void lone_goalie(caller_t& ca, World world, Player player);

				/**
				 * Goalie Move
				 *
				 * Intended for goalie use
				 *
				 * Move the goalie to this location. If the ball is dangerously
				 * moving towards the net, then rush to defend it.
				 */
				void goalie_move(caller_t& ca, World world, Player player, Point dest);

				/**
				 * Goalie Move Direct
				 *
				 * Intended for goalie use
				 *
				 * Move the goalie to this location. Don't care what the ball is
				 * doing.
				 */
				void goalie_move_direct(caller_t& ca, World world, Player player, Point dest);
			}
		}
	}
}