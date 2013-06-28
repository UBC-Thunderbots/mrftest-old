#ifndef AI_HL_STP_EVALUATION_INDIRECT_CHERRY_H
#define AI_HL_STP_EVALUATION_INDIRECT_CHERRY_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "util/cacheable.h"
#include "util/param.h"
#include <array>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				Point cherry_pivot(World world);

				bool cherry_at_point(World world, Player player);

				/**
				 * returns a passee position for passing
				 */
			}
		}
	}
}

#endif

