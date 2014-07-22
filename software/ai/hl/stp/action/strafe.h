#ifndef AI_HL_STP_ACTION_STRAFE_H
#define AI_HL_STP_ACTION_STRAFE_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Strafe in a particular direction. This should be called each tick.
				 * Orients in original direction.
				 */
				void strafe(Player player, const Point dir);

				/**
				 * Strafe in a particular direction. This should be called each tick.
				 * Orients towards an angle.
				 */

				void strafe(Player player, const Point dir, const Angle face);
				/**
				 * Strafe in a particular direction. This should be called each tick.
				 * Orients towards a point.
				 */
				void strafe(Player player, const Point dir, const Point face);

			}
		}
	}
}

#endif

