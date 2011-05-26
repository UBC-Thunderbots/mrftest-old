#ifndef AI_HL_STP_TACTIC_CM_BALL_H
#define AI_HL_STP_TACTIC_CM_BALL_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/cm_coordinate.h"

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
				Tactic::Ptr tpass(const World &world);

				/**
				 * Receive Pass
				 */
				Tactic::Ptr treceive_pass(const World &world);

				/**
				 * Dribble to Shoot
				 */
				Tactic::Ptr tdribble_to_shoot(const World &world);

				/**
				 * Dribble to Region
				 */
				Tactic::Ptr tdribble_to_region(const World &world, TRegion _region);

				/**
				 * Spin to Region
				 */
				Tactic::Ptr tspin_to_region(const World &world, TRegion _region);

				/**
				 * Receive ball deflection
				 */
				Tactic::Ptr treceive_deflection(const World &world);

			}
		}
	}
}

#endif

