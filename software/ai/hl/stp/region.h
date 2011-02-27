#ifndef AI_HL_STP_REGION_H
#define AI_HL_STP_REGION_H

#include "geom/rect.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Describes a rectangular region.
			 * See STP paper section 5.2.3 (b)
			 */
			class Region {
				public:
					/**
					 * Creates a rectangle from the two,
					 * possibly moving independently, regions.
					 */
					Region(Coordinate a, Coordinate b) : r1(a), r2(b) {
					}

					/**
					 * Creates a rectangular region of zero width and height.
					 */
					Region(Coordinate a) : r1(a), r2(a) {
					}

					~Region() {
					}

					/**
					 * Computes and returns a rectangle.
					 */
					Rect operator()() const {
						return Rect(r1(), r2());
					}

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

#endif

