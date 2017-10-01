/*
 * intercept.h
 *
 *  Created on: Dec 2, 2015
 *      Author: mathew
 */

#ifndef AI_HL_STP_EVALUATION_INTERCEPT_H_
#define AI_HL_STP_EVALUATION_INTERCEPT_H_

#include "ai/hl/stp/world.h"
#include "geom/point.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Evaluation
{
double getBestIntercept(
    World world, Player interceptor, Point predicted_ball_pos, double time);
Point quickest_intercept_position(World world, Player player);
double time_to_intercept(Player player, Point target);
}
}
}
}

#endif /* AI_HL_STP_EVALUATION_INTERCEPT_H_ */
