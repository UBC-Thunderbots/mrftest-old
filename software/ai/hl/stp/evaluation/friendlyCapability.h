/*
 * friendlyCapability.h
 *
 *  Created on: 2015-01-10
 *      Author: cheng
 */



#ifndef FRIENDLYCAPABILITY_H_
#define FRIENDLYCAPABILITY_H_

#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/world.h"
#include "geom/point.h"

using namespace AI::HL::STP::GradientApproach;

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {

				
				double getFriendlyCapability(PassInfo::worldSnapshot snapshot, Point dest, double t_delay, double ball_vel);
			}
		}
	}
}


#endif /* FRIENDLYCAPABILITY_H_ */
