#pragma once

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move
				 *
				 * Not intended for goalie use
				 *
				 * Move to a particular location and stop. Orient the player
				 * towards the ball.
				 */
				void move(World world, Player player, Point dest);

				/**
				 * Move
				 *
				 * Not intended for goalie use
				 *
				 * Move to a particular location and stop. Orient the player
				 * towards a particular direction.
				 */
				void move(World world, Player player, Point dest, Angle orientation);

				/**
				 * Move
				 *
				 * Not intended for goalie use
				 *
				 * Move to a particular location with a low velocity and stop.
				 * Orient the player towards the ball. MOVE_CAREFUL flag is set.
				 */
				void move_careful(World world, Player player, Point dest);
			}
		}
	}
}
