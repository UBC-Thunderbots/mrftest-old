#ifndef AI_HL_STP_EVALUATION_INDIRECTCHIP_H_
#define AI_HL_STP_EVALUATION_INDIRECTCHIP_H_

#include "ai/hl/stp/world.h"
#include "geom/point.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/* Returns the point to which the player taking the friendly
				 * indirect kick should chip to, to chip over the first blocker
				 *
				 */
				Point indirect_chip_target(World world, Player player);
			}
		}
	}

}



#endif
