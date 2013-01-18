#ifndef AI_HL_STP_TACTIC_TDEFEND_H
#define AI_HL_STP_TACTIC_TDEFEND_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Tri_attack offence
				 */
				Tactic::Ptr tri_attack_active(World world, unsigned i);
				/**
				 * Defend a line
				 * If p1_ == p2_ it'll be equivalent as defending a point
				 */
				
			}
		}
	}
}

#endif

