/*
*  ratepass.cpp
*
*   Created on: 2015-03-07
*       Author: cheng
*/

#include "ai/hl/stp/gradient_approach/ratepass.h"
#include <iostream>
#include "ai/hl/stp/evaluation/enemy_risk.h"
#include "ai/hl/stp/evaluation/friendly_capability.h"
#include "ai/hl/stp/evaluation/move.h"
#include "ai/hl/stp/evaluation/static_position_quality.h"
#include "ai/hl/stp/gradient_approach/PassInfo.h"

using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace AI
{
namespace HL
{
namespace STP
{
namespace GradientApproach
{
double ratePass(
    PassInfo::worldSnapshot snapshot, Point target, double time_delay,
    double ball_velocity)
{
    double pass_quality =
        Evaluation::getStaticPositionQuality(snapshot, target);

    if (snapshot.passee_positions.size() > 0)
    {
        // prefer locations we can get to
        pass_quality =
            pass_quality * (Evaluation::getFriendlyCapability(
                               snapshot, target, time_delay, ball_velocity));
    }

    if (snapshot.enemy_positions.size() > 0)
    {
        // avoid areas enemy robots can get to
        pass_quality = pass_quality *
                       (1 - Evaluation::getRatePassEnemyRisk(
                                snapshot, target, time_delay, ball_velocity));
    }

    double shoot_score = Evaluation::get_passee_shoot_score(snapshot, target);
    pass_quality =
        pass_quality *
        (0.5 +
         0.5 * shoot_score);  // give some importance to shooting (but not all)

    double dist = (snapshot.passer_position - target).len();
    pass_quality =
        pass_quality /
        (1 + std::exp(3 * (1 - dist)));  // prefer passes more than a metre away
    pass_quality =
        pass_quality /
        (1 + std::exp(
                 200 * (-time_delay +
                        0.3)));  // prefer passes more than 0.3 seconds away
    pass_quality =
        pass_quality /
        (1 +
         std::exp(
             200 *
             (2.5 - ball_velocity)));  // strict requirement that ball vel > 2.5
    pass_quality =
        pass_quality /
        (1 +
         std::exp(
             200 *
             (-6 + ball_velocity)));  // strict requirement that ball vel < 6

    return pass_quality;
}
}
}
}
}
