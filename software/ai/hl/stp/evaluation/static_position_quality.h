/*
 * StaticPositionQuality.h
 *
 *  Created on: 2015-01-10
 *      Author: cheng
 */

#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/world.h"
#include "geom/point.h"

#ifndef STATICPOSITIONQUALITY_H_
#define STATICPOSITIONQUALITY_H_

using namespace AI::HL::STP::GradientApproach;

namespace AI
{
namespace HL
{
namespace STP
{
namespace Evaluation
{
double getStaticPositionQuality(PassInfo::worldSnapshot snapshot, Point dest);
}
}
}
}

#endif /* STATICPOSITIONQUALITY_H_ */
