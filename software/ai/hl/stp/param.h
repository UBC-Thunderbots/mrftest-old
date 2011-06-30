#ifndef AI_HL_STP_PARAM_H
#define AI_HL_STP_PARAM_H

#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			extern DoubleParam min_shoot_region;

			namespace Action{
				//	these are pass specific
				// needs comments
				extern	DoubleParam alpha;
				extern DoubleParam pass_threshold;
				extern DoubleParam pass_speed;
			}


		}
	}
}

#endif

