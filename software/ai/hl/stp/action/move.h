#ifndef AI_HL_STP_ACTION_MOVE_H
#define AI_HL_STP_ACTION_MOVE_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Move to a particular location and stop.
				 * Orient the player towards the ball.
				 */
				void move(World world, Player player, const Point dest);

				/**
				 * Move to a particular location,
				 * orient towards the ball,
				 * ending in desired velocity.
				 */
				void move(World world, Player player, const Point dest, const Point vel);

				/**
				 * Move to a particular location and stop.
				 * Orient the player towards a particular direction.
				 */
				void move(Player player, const Angle orientation, const Point dest);

				/**
				 * Move to a particular location,
				 * orient the player towards a particular direction,
				 * and ending in desired velocity.
				 */
				void move(Player player, const Angle orientation, const Point dest, const Point vel);

				/**
				 * Move to a particular location and stop.
				 * Orient the player towards the ball.
				 */
				void move_careful(World world, Player player, const Point dest);
			}
		}
	}
}

#endif

