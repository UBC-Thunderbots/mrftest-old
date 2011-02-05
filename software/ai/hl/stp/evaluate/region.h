#ifndef AI_HL_STP_EVALUATE_REGION_H
#define AI_HL_STP_EVALUATE_REGION_H

#include "ai/hl/world.h"
#include "ai/hl/stp/evaluate/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Describes a rectangular region.
				 * See STP paper section 5.2.3 (b)
				 */
				class Region {
					public:
						/**
						 * Creates a rectangle starting from orig.
						 * If the width or height is negative,
						 * the rectangle is flipped accordingly.
						 */
						explicit Region(Coordinate orig, double w, double h);

						/**
						 * Creates a moving rectangle from the two specified regions.
						 * The two coordinates need not be based on the same object.
						 */
						explicit Region(Coordinate a, Coordinate b);

						/**
						 * Creates a static rectangle from the two specified regions.
						 */
						explicit Region(Point a, Point b);

					protected:
						/**
						 * The two points representing this rectangle.
						 * Does not have to be in any particular orientation.
						 */
						Coordinate r1, r2;
				};
			}
		}
	}
}
	
#endif

