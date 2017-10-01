/*
 *  Created on: 2015-01-24
 *      Author: cheng
 */

#ifndef ENEMYRISK_H_
#define ENEMYRISK_H_

#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/world.h"
#include "geom/point.h"

using namespace AI::HL::STP::GradientApproach;

namespace AI
{
namespace HL
{
namespace STP
{
namespace Evaluation
{
double getRatePassEnemyRisk(
    PassInfo::worldSnapshot snapshot, Point destination, double delay_time,
    double kickSpeed);
double dangerInterception(
    PassInfo::worldSnapshot snapshot, Point destination, double delay_time,
    double kickSpeed);
}
} /* namespace STP */
} /* namespace HL */
} /* namespace AI */

#endif /* ENEMYRISK_H_ */
