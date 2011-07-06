#ifndef AI_HL_STP_TACTIC_PASS_RAY_H
#define AI_HL_STP_TACTIC_PASS_RAY_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Pass using ray.
				 */
				Tactic::Ptr passer_ray(const World &world);
			}
		}
	}
}

#endif

