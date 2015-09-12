/*
 * optimizepass.h
 *
 *  Created on: 2015-03-07
 *      Author: cheng
 */

#ifndef OPTIMIZEPASS_H_
#define OPTIMIZEPASS_H_

#include "passMainLoop.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "ai/hl/stp/gradient_approach/PassInfo.h"

#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			namespace GradientApproach {

			std::vector<double> optimizePass(PassInfo::worldSnapshot snapshot, Point start_target, double start_t_delay, double start_shoot_vel, unsigned int max_func_evals);

			std::vector<double> testOptimizePass(PassInfo::worldSnapshot snapshot, Point start_target, double start_t_delay, double start_shoot_vel, unsigned int max_func_evals);

			std::vector<double> approximateGradient(PassInfo::worldSnapshot snapshot, std::vector<double> params,
													double step_size, double current_func_val, int num_params, std::vector<double> weights);
			} /* namespace GradientApproach */
		} /* namespace STP */
	} /* namespace HL */
} /* namespace AI */

#endif /* OPTIMIZEPASS_H_ */
