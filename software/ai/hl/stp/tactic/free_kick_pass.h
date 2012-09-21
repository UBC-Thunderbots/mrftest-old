#ifndef AI_HL_STP_TACTIC_FREE_KICK_PASS_H
#define AI_HL_STP_TACTIC_FREE_KICK_PASS_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * For now, robot moves towards ball, rotates 30 degrees below, then 30 degrees above,
				 * then to shooting position and shoots to target.
				 *
				 * \param[in] target location to aim at
				 *
				 * \param[in] chip whether to chip or kick
				 *
				 * \param[in] speed_ratio ratio of max kick or chip power from 0 to 1
				 */
				Tactic::Ptr free_kick_pass(AI::HL::W::World world, const Point target, bool chip = false, double speed_ratio = 1.0);
			}
		}
	}
}

#endif
