#ifndef AI_HL_STP_TACTIC_MOVE_STOP_H
#define AI_HL_STP_TACTIC_MOVE_STOP_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Move to a dynamic location.
				 */
				Tactic::Ptr move_stop(const World &world, int playerIndex);

				class StopLocations : public Cacheable<std::vector<Point>, CacheableNonKeyArgs<const AI::HL::W::World &>, CacheableKeyArgs<> > {
					protected:
						std::vector<Point> compute(const World &w);
				};

				extern StopLocations stop_locations;
			}
		}
	}
}

#endif
