#ifndef AI_HL_STP_ACTION_PIVOT_H
#define AI_HL_STP_ACTION_PIVOT_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Go chase after the ball and pivot towards the direction of target
				 */
				void intercept_pivot(World world, Player player, const Point target,const double radius =0);

				/**
				 * Pivot around the ball until the player is oriented in the direction of the target.
				 *
				 *  \param[in] target Used to determine the direction the player should be facing when player finishes pivoting.
				 *
				 *  \param[in] radius The distance player stays from the ball while pivoting.
				 */
				void pivot(World world, Player player, const Point target, const double radius = 0);
			}
		}
	}
}

#endif

