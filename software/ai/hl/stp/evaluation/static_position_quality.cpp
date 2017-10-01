/*
 *  Created on: 2015-01-10
 *      Author: cheng
 */

#include "ai/hl/stp/evaluation/static_position_quality.h"
#include <math.h>
#include "ai/hl/world.h"
#include "geom/point.h"

using namespace AI::HL::W;
using namespace AI::HL::STP;
using namespace AI::HL::STP::GradientApproach;

double AI::HL::STP::Evaluation::getStaticPositionQuality(
    PassInfo::worldSnapshot snapshot, Point dest)
{
    double positionQuality = 1;
    double length          = snapshot.field_length / 2;
    double width           = snapshot.field_width / 2;
    if (dest.x >= 0)
    {
        positionQuality =
            positionQuality / (1 + std::exp(15 * (dest.x - (length - 0.3))));
    }
    else if (dest.x < 0)
    {
        positionQuality =
            positionQuality / (1 + std::exp(15 * (-dest.x - (length - 0.3))));
    }
    if (dest.y >= 0)
    {
        positionQuality =
            positionQuality / (1 + std::exp(15 * (dest.y - (width - 0.4))));
    }
    else if (dest.y < 0)
    {
        positionQuality =
            positionQuality / (1 + std::exp(15 * (-dest.y - (width - 0.4))));
    }

    Point a = Point(
        snapshot.friendly_goal.x - dest.x, snapshot.friendly_goal.y - dest.y);
    positionQuality =
        positionQuality * (1 - std::exp(-0.1 * (std::pow(2, a.len()))));

    return positionQuality;
}
