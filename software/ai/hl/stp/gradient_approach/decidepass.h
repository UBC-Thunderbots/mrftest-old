/*
 * decidepass.h
 *
 *  Created on: 2015-03-05
 *      Author: cheng
 */

#ifndef DECIDEPASS_H_
#define DECIDEPASS_H_

#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "geom/util.h"

using namespace AI::HL::W;

namespace AI
{
namespace HL
{
namespace STP
{
namespace GradientApproach
{
Point getTarget(World world);
double getShot_Velocity(World world);
double getTime_Delay(World world);

} /* namespace gradientApproach */
} /* namespace STP */
} /* namespace HL */
} /* namespace AI */

#endif /* DECIDEPASS_H_ */
