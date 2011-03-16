#ifndef AI_HL_STP_TACTIC_MOVEMENT_BENCHMARK_H
#define AI_HL_STP_TACTIC_MOVEMENT_BENCHMARK_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Move to a dynamic location.
				 */
				Tactic::Ptr movement_benchmark(const World &world);
			}
		}
	}
}

#endif

