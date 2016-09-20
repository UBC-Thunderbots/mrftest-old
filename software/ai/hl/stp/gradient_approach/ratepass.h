/*
 * ratepass.h
 *
 *  Created on: 2015-03-07
 *      Author: cheng
 */

#ifndef RATEPASS_H_
#define RATEPASS_H_

#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
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

				double ratePass(PassInfo::worldSnapshot snapshot, Point target, double time_delay, double ball_velocity);

			} /* namespace Evaluation */
		} /* namespace STP */
	} /* namespace HL */
} /* namespace AI */

#endif /* RATEPASS_H_ */
