#ifndef AI_HL_STP_TACTIC_TACTIC_H
#define AI_HL_STP_TACTIC_TACTIC_H

#include "ai/hl/world.h"
#include "util/byref.h"
#include "util/registerable.h"

namespace AI {
	namespace HL {
		namespace STP {

			/**
			 * A tactic is a layer in the STP paradigm.
			 *
			 * A stateless tactic is always preferred for simplicity.
			 * However, tactic can be stateful.
			 * In such a case, a play will have to store the tactic.
			 */
			class Tactic : public ByRef {
				public:
					typedef RefPtr<Tactic> Ptr;

					Tactic(AI::HL::W::World& world);

					~Tactic();

					/**
					 * Scoring function
					 * to indicate how preferable this particular player is.
					 *
					 * \return between 1 and 0, indicating the preference.
					 */
					virtual double score(AI::HL::W::Player::Ptr player) const = 0;

					/**
					 * Drive some actual players.
					 */
					virtual void tick(AI::HL::W::Player::Ptr player) = 0;

				protected:
					AI::HL::W::World& world;
			};

		}
	}
}

#endif

