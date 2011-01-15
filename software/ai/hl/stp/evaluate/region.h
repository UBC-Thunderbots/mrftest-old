#ifndef AI_HL_STP_EVALUATE_REGION_H
#define AI_HL_STP_EVALUATE_REGION_H

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluate {
				/**
				 * Types of regions.
				 * See STP paper section 5.2.3 (b)
				 */
				enum RegionType {
					REGION_RECTANGLE,

					REGION_CIRCLE,
				};

				/**
				 * Describes a region.
				 * See STP paper section 5.2.3 (b)
				 * Can be a rectangle or circle.
				 *
				 * It should use the Coordinate class to allow it to be dynamically changing.
				 */
				class Region {
				};
			}
		}
	}
}
	
#endif

