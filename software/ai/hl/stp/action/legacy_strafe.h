#pragma once
#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Strafe
				 *
				 * Not intended for goalie use
				 *
				 * Strafe in a particular direction. This should be called each
				 * tick. Orients in original direction.
				 */
				void strafe(Player player, const Point dir);

				/**
				 * Strafe
				 *
				 * Not intended for goalie use
				 *
				 * Strafe in a particular direction. This should be called each
				 * tick. Orients towards an angle.
				 */

				void strafe(Player player, const Point dir, const Angle face);

				/**
				 * Strafe
				 *
				 * Not intended for goalie use
				 *
				 * Strafe in a particular direction. This should be called each
				 * tick. Orients towards a point.
				 */
				void strafe(Player player, const Point dir, const Point face);

				void strafe_dribble(Player player, const Point dir, const Point face);
			}
		}
	}
}
