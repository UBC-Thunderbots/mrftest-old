#ifndef AI_HL_STP_PARAM_H
#define AI_HL_STP_PARAM_H

#include "util/param.h"

namespace AI {
	namespace HL {
		namespace STP {

			extern DoubleParam min_pass_dist;

			extern DoubleParam min_shoot_region;

			extern DoubleParam passee_angle_threshold;

			extern DoubleParam shoot_accuracy;

			extern DoubleParam shoot_width;

			namespace Action {
				//	these are pass specific
				// needs comments
				extern	DoubleParam alpha;

				extern DoubleParam pass_speed;

				extern DoubleParam passer_angle_threshold;

				extern DoubleParam target_region_param;
			}
			
			namespace Tactic {
				extern BoolParam random_penalty_goalie;
			}
		}
	}
}

#endif

