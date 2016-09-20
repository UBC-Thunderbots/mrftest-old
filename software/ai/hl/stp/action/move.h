#pragma once

#include "ai/hl/stp/action/action.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move
				 *
				 * Move to a particular location and stop. Orient the player
				 * towards the ball.
				 */
				void move(caller_t& ca, World world, Player player, Point dest);

				/**
				 * Move
				 *
				 * Move to a particular location and stop. Orient the player
				 * towards a particular direction.
				 */
				void move(caller_t& ca, World world, Player player, Angle orientation, Point dest);

				void move_dribble(caller_t& ca, World world, Player player, Angle orientation, Point dest);

				/**
				 * Move
				 *
				 * Move to a particular location with a low velocity and stop.
				 * Orient the player towards the ball. MOVE_CAREFUL flag is set.
				 */
				void move_careful(caller_t& ca, World world, Player player, Point dest);
			}
		}
	}
}
