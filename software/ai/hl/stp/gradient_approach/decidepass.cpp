/*
 * decidepass.cpp
 *
 *  Created on: 2015-03-05
 *      Author: cheng
 */
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "decidepass.h"

using namespace AI::HL::W;

namespace AI {
namespace HL {
namespace STP {
namespace GradientApproach {

Point getTarget(World world){
	Point target = Point(0,0);
	return target;
}

double getShot_Velocity(World world){
	double shot_velocity = 5;
	return shot_velocity;
}

double getTime_Delay(World world){
	double time_delay = 0.5;
	return time_delay;
}

} /* namespace gradientApproach */
} /* namespace STP */
} /* namespace HL */
} /* namespace AI */
