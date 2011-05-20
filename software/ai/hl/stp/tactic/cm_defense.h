#ifndef AI_HL_STP_TACTIC_CM_DEFENSE_H
#define AI_HL_STP_TACTIC_CM_DEFENSE_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/cm_coordinate.h"

#define DEFENSE_OFF_BALL 0.09
#define MARK_OFF_OPPONENT 0.270 

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Defend a line
				 */
				Tactic::Ptr tdefend_line(const World &world, const TCoordinate _p1, const TCoordinate _p2, const double _distmin, const double _distmax);

				/**
				 * Defend a point
				 */
				Tactic::Ptr tdefend_point(const World &world, const TCoordinate _center, const double _distmin, const double _distmax);

				/**
				 * Defend a lane
				 */
				Tactic::Ptr tdefend_lane(const World &world, const TCoordinate _p1, const TCoordinate _p2);

				/*
				
				Tactic::Ptr block(const World &world);

				Tactic::Ptr mark(const World &world);
				*/
			}
		}
	}
}

#endif

