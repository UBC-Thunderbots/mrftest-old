#ifndef AI_HL_STP_PARAM_H
#define AI_HL_STP_PARAM_H

// #include "util/param.h"
#include "ai/param.h"

namespace AI {
	namespace HL {
		namespace STP {
			extern DoubleParam min_pass_dist;

			extern DegreeParam min_shoot_region;

			extern DegreeParam passee_angle_threshold;

			extern RadianParam shoot_accuracy;

			extern DoubleParam shoot_width;

			extern DoubleParam goal_avoid_radius;

			namespace Action {
				// these are pass specific
				// needs comments
				extern DoubleParam alpha;

				extern DoubleParam pass_speed;

				extern DegreeParam passer_angle_threshold;

				extern DoubleParam target_region_param;
			}

			namespace Tactic {
				extern BoolParam random_penalty_goalie;

				extern DegreeParam separation_angle;
			}

			namespace Test {
				extern BoolParam enable0;
				extern BoolParam enable1;
				extern BoolParam enable2;
				extern BoolParam enable3;
				extern BoolParam enable4;
				extern BoolParam enable5;
				extern BoolParam enable6;
				extern BoolParam enable7;
				extern BoolParam enable8;
				extern BoolParam enable9;
				extern BoolParam enable10;
				extern BoolParam enable11;

				const bool robot_enabled[] = {
					enable0,
					enable1,
					enable2,
					enable3,
					enable4,
					enable5,
					enable6,
					enable7,
					enable8,
					enable9,
					enable10,
					enable11,
				};
			}
		}
	}
}

#endif

