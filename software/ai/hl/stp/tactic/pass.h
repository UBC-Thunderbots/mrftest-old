#ifndef AI_HL_STP_TACTIC_PASS_H
#define AI_HL_STP_TACTIC_PASS_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				
				/**
				 * The STP suggested method for passing.
				 * See instructions in passer_ready_position.
				 *
				 * \param [in] target where the passee should be.
				 */
				Tactic::Ptr passer_shoot(const World &world);

				/**
				 * The STP suggested method for passing.
				 * See instructions in passer_ready_position.
				 *
				 * \param [in] pos where the passee should be.
				 */
				Tactic::Ptr passee_receive(const World &world);
				
				Tactic::Ptr def_passee_receive(const World &world);
			}
		}
	}
}

#endif

