#ifndef AI_HL_STP_TACTIC_PASS_H
#define AI_HL_STP_TACTIC_PASS_H

#include "ai/hl/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Passing requires some a passer and passee.
				 * To use this correctly, create a reference pointer to this instance.
				 */
				class Pass : public ByRef {
					public:
						typedef RefPtr<Pass> Ptr;

						Pass(AI::HL::W::World &world);

						/**
						 * Returns a tactic instance for passer.
						 */
						Tactic::Ptr passer();

						/**
						 * Returns a tactic instance for passee.
						 */
						Tactic::Ptr passee();

					protected:
						/**
						 * The actual work is done in this function.
						 */
						void tick();
				};
			}
		}
	}
}

#endif

