#ifndef AI_HL_STP_BALLER_H
#define AI_HL_STP_BALLER_H

#include "ai/hl/stp/world.h"
#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			Player::CPtr select_friendly_baller(const World &world);
		}
	}
}

#endif

