#ifndef AI_HL_STP_EVALUATION_EVALUATION_H
#define AI_HL_STP_EVALUATION_EVALUATION_H

#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * A hack that allows evaluations that update every tick.
				 */
				extern sigc::signal<void> signal_tick;
			}
		}
	}
}

#endif

