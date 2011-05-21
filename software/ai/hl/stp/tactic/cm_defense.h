#ifndef AI_HL_STP_TACTIC_CM_DEFENSE_H
#define AI_HL_STP_TACTIC_CM_DEFENSE_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/cm_coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Defend a line
				 */
				Tactic::Ptr tdefend_line(const World &world, TCoordinate _p1, TCoordinate _p2, double _distmin, double _distmax);

				/**
				 * Defend a point
				 */
				Tactic::Ptr tdefend_point(const World &world, TCoordinate _center, double _distmin, double _distmax);

				/**
				 * Defend a lane
				 */
				Tactic::Ptr tdefend_lane(const World &world, TCoordinate _p1, TCoordinate _p2);

				/*

				   Tactic::Ptr block(const World &world);

				   Tactic::Ptr mark(const World &world);
				 */
			}
		}
	}
}

#endif

