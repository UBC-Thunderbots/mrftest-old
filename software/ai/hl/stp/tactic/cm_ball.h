#ifndef AI_HL_STP_TACTIC_CM_BALL_H
#define AI_HL_STP_TACTIC_CM_BALL_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/region.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {				
				/**
				 * Shoot
				 */
				Tactic::Ptr tshoot(const World &world);

				/**
				 * Steal 
				 */
				Tactic::Ptr tsteal(const World &world);

				/**
				 * Clear
				 */
				Tactic::Ptr tclear(const World &world);

				/**
				 * Active Defense
				 */
				Tactic::Ptr tactive_def(const World &world);

				/**
				 * Pass
				 */
				Tactic::Ptr tpass(const World &world, const Coordinate _target);

				/**
				 * Receive Pass
				 */
				Tactic::Ptr treceive_pass(const World &world, const Coordinate _target);

				/**
				 * Dribble to Region
				 */
				Tactic::Ptr tdribble_to_region(const World &world, Region _region);

				/**
				 * Spin to Region
				 */
				Tactic::Ptr tspin_to_region(const World &world, Region _region);

			}
		}
	}
}

#endif

