#pragma once
#include "ai/hl/world.h"

namespace AI
{
namespace HL
{
namespace STP
{
using namespace AI::HL::W;

/**
 * The max number of players we can assign roles
 */
constexpr unsigned int TEAM_MAX_SIZE = 6;

/**
 * The max speed the ball can be kicked
 */
extern DoubleParam BALL_MAX_SPEED;

/**
 * Distance from the penalty mark
 * updated at robocup 2018 to be 0.8. need to check actual value
 */
constexpr double DIST_FROM_PENALTY_MARK = 0.8; //0.4;
}
}
}
