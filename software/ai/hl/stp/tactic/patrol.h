#ifndef AI_HL_STP_TACTIC_PATROL_H
#define AI_HL_STP_TACTIC_PATROL_H

#include "ai/hl/world.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Used for going back and forth between 2 destinations.
				 * A play must store this tactic in order to use this correctly.
				 */
				class Patrol : public Tactic {
					public:
						Patrol(AI::HL::W::World &world, Point w1, Point w2);

						double score(AI::HL::W::Player::Ptr player) const;

						void execute(AI::HL::W::Player::Ptr player);
						
						void set_flags(const unsigned int f) {
							flags = f;
						}
					protected:
						Point p1, p2;
						bool goto_target1;
						unsigned int flags;
				};
			}
		}
	}
}

#endif

