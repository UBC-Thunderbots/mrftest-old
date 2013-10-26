#ifndef AI_HL_STP_TACTIC_PASS_RAY_H
#define AI_HL_STP_TACTIC_PASS_RAY_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Passer Ray
				 * Active Tactic
				 * Pass using ray; A tactic that uses the best_shoot_ray() evaluation to determine the best passing angle and shoots with that angle. 
				 */
				Tactic::Ptr passer_ray(World world);
			}
		}
	}
}

#endif

