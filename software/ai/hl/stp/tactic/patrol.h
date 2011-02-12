#ifndef AI_HL_STP_TACTIC_PATROL_H
#define AI_HL_STP_TACTIC_PATROL_H

#include "ai/hl/world.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				class Patrol : public Tactic {
					public:
						Patrol(AI::HL::W::World &world, Coordinate w1, Coordinate w2);

						double score(AI::HL::W::Player::Ptr player) const;

						void execute(AI::HL::W::Player::Ptr player);
						
						void set_flags(const unsigned int f) {
							flags = f;
						}
					protected:
						Coordinate p1, p2;
						bool goto_target1;
						unsigned int flags;
				};
			}
		}
	}
}

#endif

