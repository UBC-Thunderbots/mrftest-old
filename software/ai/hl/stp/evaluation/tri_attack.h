#ifndef AI_HL_STP_EVALUATION_TRI_ATTACK_H
#define AI_HL_STP_EVALUATION_TRI_ATTACK_H

#include "ai/hl/stp/world.h"

#include <array>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {


				/**
				 * return point to where player needs to go in a tri-attack
				 */
				Point tri_attack_evaluation(World world);
			}
		}
	}
}

#endif

